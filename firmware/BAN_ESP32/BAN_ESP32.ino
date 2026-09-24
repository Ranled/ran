// ============================================================
// R.A.N. — Raian AI Network (ESP32 Firmware v3.0)
// Complete Offline Local Conversational AI Companion Engine
// Hardware: ESP32 + microSD Card + BLE GATT (Web HCI) + USB Serial
// No Cloud. No Raspberry Pi. No external LLM required.
//
// INSTRUCTIONS FOR ARDUINO IDE:
// 1. Install Library: Tools -> Manage Libraries -> search "ArduinoJson" (v6 or v7)
// 2. Select Board: Tools -> Board -> ESP32 Arduino -> "ESP32 Dev Module"
// 3. Partition Scheme: Tools -> Partition Scheme -> "Huge APP (3MB No OTA/1MB SPIFFS)"
// 4. Connect ESP32 via USB and click Upload (Ctrl+U)
// 5. Open Serial Monitor at 115200 baud or Web HCI at http://localhost:5173/
// ============================================================

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

// ─── Hardware Pin Configuration ──────────────────────────────
#define SD_CS_PIN       5
#define SD_MOSI_PIN     23
#define SD_MISO_PIN     19
#define SD_SCK_PIN      18

// ─── BLE GATT UUIDs (must match Web HCI exactly) ──────────────
#define RAN_SERVICE_UUID  "4fafc201-1fb5-459e-8fcc-c5c9c3319141"
#define RAN_TX_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a8" // Notify (ESP32 -> Web)
#define RAN_RX_UUID       "6e400002-b5a3-f393-e0a9-e50e24dcca9e" // Write  (Web -> ESP32)

#define BLE_DEVICE_NAME   "RAN-ESP32"
#define BLE_CHUNK_SIZE    512
#define BLE_TERMINATOR    '\x00'
#define BLE_MAX_MSG_LEN   2048

// ─── microSD Directories & Paths (Primary: /RAN, Fallback: /BAN) ─
#define RAN_DIR_PRIMARY      "/RAN"
#define RAN_DIR_FALLBACK     "/BAN"

#define MAX_TOKENS           32
#define MAX_CONTEXT_TURNS    6
#define MAX_MEMORIES_IN_RAM  64
#define MAX_KNOWLEDGE_TOPICS 32
#define MEMORY_JSON_CAPACITY 8192

// ─── Intent Enumeration ───────────────────────────────────────
enum Intent {
    INTENT_UNKNOWN = 0,
    INTENT_GREETING,
    INTENT_FAREWELL,
    INTENT_IDENTITY,
    INTENT_CAPABILITY,
    INTENT_MEMORY_QUERY,
    INTENT_MEMORY_SAVE,
    INTENT_MEMORY_DELETE,
    INTENT_PROJECT_QUERY,
    INTENT_TASK_QUERY,
    INTENT_KNOWLEDGE_QUERY,
    INTENT_TIME_QUERY,
    INTENT_DATE_QUERY,
    INTENT_STATUS_QUERY,
    INTENT_HELP,
    INTENT_THANKS,
    INTENT_CONFIRMATION,
    INTENT_QUESTION
};

const char* intentToString(Intent intent) {
    switch (intent) {
        case INTENT_GREETING:        return "GREETING";
        case INTENT_FAREWELL:        return "FAREWELL";
        case INTENT_IDENTITY:        return "IDENTITY";
        case INTENT_CAPABILITY:      return "CAPABILITY";
        case INTENT_MEMORY_QUERY:    return "MEMORY_QUERY";
        case INTENT_MEMORY_SAVE:     return "MEMORY_SAVE";
        case INTENT_MEMORY_DELETE:   return "MEMORY_DELETE";
        case INTENT_PROJECT_QUERY:   return "PROJECT_QUERY";
        case INTENT_TASK_QUERY:      return "TASK_QUERY";
        case INTENT_KNOWLEDGE_QUERY: return "KNOWLEDGE_QUERY";
        case INTENT_TIME_QUERY:      return "TIME_QUERY";
        case INTENT_DATE_QUERY:      return "DATE_QUERY";
        case INTENT_STATUS_QUERY:    return "STATUS_QUERY";
        case INTENT_HELP:            return "HELP";
        case INTENT_THANKS:          return "THANKS";
        case INTENT_CONFIRMATION:    return "CONFIRMATION";
        case INTENT_QUESTION:        return "QUESTION";
        default:                     return "UNKNOWN";
    }
}

// ─── AI Response Structure ────────────────────────────────────
struct AiResponse {
    String text;
    String mood;       // "idle", "wave", "talking", "happy", "wondering", "shock", "angry", "sad", "walking", "running"
    Intent intent;
    float  confidence;
};

// ─── Forward Declarations ─────────────────────────────────────
void onBLEMessage(const String& json);
void sendBLEMessage(const String& json);
void sendBLEResponse(const AiResponse& resp);
void sendBLEStatus();
void sendBLEMemoryList();
void sendBLEPersonality();
void sendBLEError(const String& code, const String& message);

// ============================================================
// 1. SD STORAGE & HARDWARE MANAGER
// ============================================================
class SDStorageManager {
public:
    bool   ready = false;
    String rootDir = RAN_DIR_PRIMARY;
    String convFile = "";

    bool begin() {
        SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
        if (!SD.begin(SD_CS_PIN)) {
            Serial.println(F("[SD] Mount failed. Check wiring (CS=5, SCK=18, MOSI=23, MISO=19)."));
            ready = false;
            return false;
        }
        if (SD.cardType() == CARD_NONE) {
            Serial.println(F("[SD] No microSD card detected."));
            ready = false;
            return false;
        }
        ready = true;

        // Auto-detect root folder: /RAN or fallback /BAN
        if (SD.exists(RAN_DIR_PRIMARY)) {
            rootDir = RAN_DIR_PRIMARY;
        } else if (SD.exists(RAN_DIR_FALLBACK)) {
            rootDir = RAN_DIR_FALLBACK;
        } else {
            rootDir = RAN_DIR_PRIMARY;
            SD.mkdir(rootDir.c_str());
        }

        ensureDirectories();
        startNewConversation();

        Serial.print(F("[SD] Card Mounted: "));
        Serial.print((uint32_t)(SD.cardSize() / (1024 * 1024)));
        Serial.print(F(" MB | Root: "));
        Serial.println(rootDir);
        return true;
    }

    String path(const String& file) {
        return rootDir + "/" + file;
    }

    void ensureDirectories() {
        if (!ready) return;
        String dirs[] = { rootDir, rootDir + "/conversations", rootDir + "/logs", rootDir + "/knowledge" };
        for (int i = 0; i < 4; i++) {
            if (!SD.exists(dirs[i].c_str())) {
                SD.mkdir(dirs[i].c_str());
            }
        }
    }

    String readFile(const String& filename) {
        if (!ready) return "";
        String fullPath = filename.startsWith("/") ? filename : path(filename);
        if (!SD.exists(fullPath.c_str())) return "";

        File f = SD.open(fullPath.c_str(), FILE_READ);
        if (!f) return "";

        String content = "";
        content.reserve(f.size() + 1);
        while (f.available()) {
            content += (char)f.read();
        }
        f.close();
        return content;
    }

    bool writeFile(const String& filename, const String& content) {
        if (!ready) return false;
        String fullPath = filename.startsWith("/") ? filename : path(filename);

        // Safe atomic write via temporary file
        String tempPath = fullPath + ".tmp";
        File f = SD.open(tempPath.c_str(), FILE_WRITE);
        if (!f) return false;

        f.print(content);
        f.flush();
        f.close();

        if (SD.exists(fullPath.c_str())) {
            SD.remove(fullPath.c_str());
        }
        return SD.rename(tempPath.c_str(), fullPath.c_str());
    }

    void startNewConversation() {
        if (!ready) return;
        convFile = rootDir + "/conversations/conv_" + String(millis()) + ".txt";
        File f = SD.open(convFile.c_str(), FILE_WRITE);
        if (f) {
            f.println(F("=== R.A.N. Conversation Session Started ==="));
            f.flush();
            f.close();
        }
    }

    void logExchange(const String& user, const String& ran) {
        if (!ready || convFile.length() == 0) return;
        File f = SD.open(convFile.c_str(), FILE_APPEND);
        if (f) {
            f.print(F("[USER]: "));
            f.println(user);
            f.print(F("[R.A.N.]: "));
            f.println(ran);
            f.println();
            f.flush();
            f.close();
        }
    }
} sdStorage;

// ============================================================
// 2. MEMORY & KNOWLEDGE MANAGEMENT
// ============================================================
struct MemoryRecord {
    int    id;
    String category;
    String key;
    String value;
    String created;
};

struct KnowledgeRecord {
    String topic;
    String keywords;
    String answer;
};

class MemoryManager {
public:
    int nextId = 1;

    int getCount() {
        if (!sdStorage.ready) return 0;
        String raw = sdStorage.readFile("memory.json");
        if (raw.length() == 0) return 0;
#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
#endif
        if (deserializeJson(doc, raw)) return 0;
        return doc["memories"].size();
    }

    int saveMemory(const String& category, const String& key, const String& value) {
        if (!sdStorage.ready) return -1;
        String raw = sdStorage.readFile("memory.json");

#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
#endif
        if (raw.length() > 0) {
            deserializeJson(doc, raw);
        }

        JsonArray arr = doc["memories"].as<JsonArray>();
        if (arr.isNull()) {
            arr = doc.createNestedArray("memories");
        }

        int maxId = 0;
        for (JsonObject m : arr) {
            int id = m["id"].as<int>();
            if (id > maxId) maxId = id;
        }

        int newId = maxId + 1;
        JsonObject item = arr.createNestedObject();
        item["id"]       = newId;
        item["type"]     = category;
        item["name"]     = key;
        item["content"]  = value;
        item["category"] = category;
        item["key"]      = key;
        item["value"]    = value;
        item["created"]  = "2026-09-25";

        String out;
        serializeJson(doc, out);
        if (sdStorage.writeFile("memory.json", out)) {
            return newId;
        }
        return -1;
    }

    bool deleteMemory(int id) {
        if (!sdStorage.ready) return false;
        String raw = sdStorage.readFile("memory.json");
        if (raw.length() == 0) return false;

#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
#endif
        if (deserializeJson(doc, raw)) return false;

        JsonArray arr = doc["memories"].as<JsonArray>();
        bool found = false;
        for (size_t i = 0; i < arr.size(); i++) {
            if (arr[i]["id"].as<int>() == id) {
                arr.remove(i);
                found = true;
                break;
            }
        }

        if (found) {
            String out;
            serializeJson(doc, out);
            return sdStorage.writeFile("memory.json", out);
        }
        return false;
    }

    bool clearMemory() {
        if (!sdStorage.ready) return false;
#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        DynamicJsonDocument doc(512);
#endif
        doc.createNestedArray("memories");
        String out;
        serializeJson(doc, out);
        return sdStorage.writeFile("memory.json", out);
    }

    // Fuzzy search across memories (Section 13: token overlap, substring, normalized words, keywords)
    String searchMemory(const String& query, float& outScore) {
        outScore = 0.0f;
        if (!sdStorage.ready) return "";
        String raw = sdStorage.readFile("memory.json");
        if (raw.length() == 0) return "";

#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
#endif
        if (deserializeJson(doc, raw)) return "";

        String lowerQuery = query;
        lowerQuery.toLowerCase();

        String bestMatch = "";
        float maxScore = 0.0f;

        JsonArray arr = doc["memories"].as<JsonArray>();
        for (JsonObject m : arr) {
            String k = m["name"].as<String>();
            if (k.length() == 0) k = m["key"].as<String>();

            String v = m["content"].as<String>();
            if (v.length() == 0) v = m["value"].as<String>();

            String c = m["type"].as<String>();
            if (c.length() == 0) c = m["category"].as<String>();

            String lk = k; lk.toLowerCase();
            String lv = v; lv.toLowerCase();
            String lc = c; lc.toLowerCase();

            float score = 0.0f;
            if (lk.length() > 0 && lowerQuery.indexOf(lk) >= 0) score += 4.0f;
            if (lv.length() > 0 && lowerQuery.indexOf(lv) >= 0) score += 3.0f;
            if (lowerQuery.length() > 2 && lv.indexOf(lowerQuery) >= 0) score += 2.5f;
            if (lc.length() > 0 && lowerQuery.indexOf(lc) >= 0) score += 1.0f;

            // Search in keywords array
            if (m.containsKey("keywords")) {
                JsonArray kws = m["keywords"].as<JsonArray>();
                for (JsonVariant kw : kws) {
                    String kwStr = kw.as<String>();
                    kwStr.toLowerCase();
                    if (lowerQuery.indexOf(kwStr) >= 0) {
                        score += 3.0f;
                    }
                }
            }

            // Word token overlap for fuzzy search
            int start = 0;
            int qlen = lowerQuery.length();
            for (int i = 0; i <= qlen; i++) {
                if (i == qlen || lowerQuery[i] == ' ') {
                    if (i - start > 2) {
                        String word = lowerQuery.substring(start, i);
                        if (lk.indexOf(word) >= 0) score += 1.5f;
                        if (lv.indexOf(word) >= 0) score += 1.0f;
                    }
                    start = i + 1;
                }
            }

            if (score > maxScore) {
                maxScore = score;
                bestMatch = k + ": " + v;
            }
        }

        outScore = maxScore;
        return bestMatch;
    }

    // Knowledge search across knowledge.json
    String searchKnowledge(const String& query, float& outScore) {
        outScore = 0.0f;
        if (!sdStorage.ready) return "";
        String raw = sdStorage.readFile("knowledge.json");
        if (raw.length() == 0) return "";

#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        DynamicJsonDocument doc(MEMORY_JSON_CAPACITY);
#endif
        if (deserializeJson(doc, raw)) return "";

        String lowerQuery = query;
        lowerQuery.toLowerCase();

        String bestAnswer = "";
        float maxScore = 0.0f;

        JsonArray arr = doc["knowledge"].as<JsonArray>();
        for (JsonObject k : arr) {
            String topic = k["topic"].as<String>();
            String ans   = k["answer"].as<String>();
            String ltopic = topic; ltopic.toLowerCase();

            float score = 0.0f;
            if (lowerQuery.indexOf(ltopic) >= 0) score += 4.0f;

            JsonArray kws = k["keywords"].as<JsonArray>();
            for (JsonVariant kw : kws) {
                String kwStr = kw.as<String>();
                kwStr.toLowerCase();
                if (lowerQuery.indexOf(kwStr) >= 0) {
                    score += 2.0f;
                }
            }

            if (score > maxScore) {
                maxScore = score;
                bestAnswer = ans;
            }
        }

        outScore = maxScore;
        return bestAnswer;
    }
} memoryManager;

// ============================================================
// 3. CONTEXT & CONVERSATION MANAGER
// ============================================================
struct ContextExchange {
    String userText;
    String botReply;
    Intent intent;
    String topic;
};

class ContextManager {
public:
    ContextExchange history[MAX_CONTEXT_TURNS];
    int count = 0;
    String currentTopic = "";

    void addExchange(const String& user, const String& bot, Intent intent, const String& topic) {
        if (count < MAX_CONTEXT_TURNS) {
            history[count++] = { user, bot, intent, topic };
        } else {
            for (int i = 0; i < MAX_CONTEXT_TURNS - 1; i++) {
                history[i] = history[i + 1];
            }
            history[MAX_CONTEXT_TURNS - 1] = { user, bot, intent, topic };
        }
        if (topic.length() > 0) {
            currentTopic = topic;
        }
    }

    String resolvePronoun(const String& input) {
        String lower = input;
        lower.toLowerCase();
        // If user says "what does it use" or "what is it" or "tell me about that"
        if (currentTopic.length() > 0 && 
           (lower.indexOf(" it ") >= 0 || lower.endsWith(" it") || lower.indexOf(" that ") >= 0 || lower.endsWith(" that") || lower.indexOf("the project") >= 0)) {
            return currentTopic + " " + input;
        }
        return input;
    }

    void clear() {
        count = 0;
        currentTopic = "";
    }
} contextManager;

// ============================================================
// 4. LOCAL AI ENGINE: INPUT PROCESSOR & INTENT DETECTOR
// ============================================================
class LocalAIEngine {
public:
    // ── Input Normalization ──────────────────────────────────
    String normalize(const String& input) {
        String s = input;
        s.toLowerCase();
        s.trim();

        // Strip punctuation
        String clean = "";
        clean.reserve(s.length());
        for (size_t i = 0; i < s.length(); i++) {
            char c = s[i];
            if (c == '!' || c == '?' || c == '.' || c == ',' || c == ';' || 
                c == ':' || c == '"' || c == '\'' || c == '(' || c == ')' || 
                c == '[' || c == ']' || c == '{' || c == '}') {
                clean += ' ';
            } else {
                clean += c;
            }
        }

        // Collapse multiple spaces
        String result = "";
        result.reserve(clean.length());
        bool lastSpace = true;
        for (size_t i = 0; i < clean.length(); i++) {
            char c = clean[i];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                if (!lastSpace) {
                    result += ' ';
                    lastSpace = true;
                }
            } else {
                result += c;
                lastSpace = false;
            }
        }
        result.trim();
        return result;
    }

    int tokenize(const String& normalized, String tokens[], int maxTokens) {
        int tokenCount = 0;
        int start = 0;
        int len = normalized.length();

        for (int i = 0; i <= len && tokenCount < maxTokens; i++) {
            if (i == len || normalized[i] == ' ') {
                if (i > start) {
                    tokens[tokenCount++] = normalized.substring(start, i);
                }
                start = i + 1;
            }
        }
        return tokenCount;
    }

    // ── Intent Detection & Scoring ───────────────────────────
    Intent detectIntent(const String& normalized, float& confidence) {
        confidence = 0.0f;
        if (normalized.length() == 0) return INTENT_UNKNOWN;

        // Keyword score table
        float scoreGreeting   = 0.0f;
        float scoreFarewell   = 0.0f;
        float scoreIdentity   = 0.0f;
        float scoreCapability = 0.0f;
        float scoreMemSave    = 0.0f;
        float scoreMemQuery   = 0.0f;
        float scoreMemDelete  = 0.0f;
        float scoreProject    = 0.0f;
        float scoreKnowledge  = 0.0f;
        float scoreStatus     = 0.0f;
        float scoreTime       = 0.0f;
        float scoreHelp       = 0.0f;
        float scoreThanks     = 0.0f;

        // 1. Explicit Memory Commands
        if (normalized.startsWith("remember that ") || normalized.startsWith("remember my ") || normalized.startsWith("store this ") || normalized.startsWith("save ")) {
            scoreMemSave += 6.0f;
        }

        if (normalized.startsWith("forget ") || normalized.indexOf("delete memory") >= 0 || normalized.indexOf("erase memory") >= 0) {
            scoreMemDelete += 6.0f;
        }

        // 2. Greetings
        if (normalized.startsWith("hello") || normalized.startsWith("hi ") || normalized == "hi" || 
            normalized.startsWith("hey") || normalized.indexOf("good morning") >= 0 || 
            normalized.indexOf("good afternoon") >= 0 || normalized.indexOf("good evening") >= 0) {
            scoreGreeting += 5.0f;
        }

        // 3. Farewell
        if (normalized.startsWith("bye") || normalized.startsWith("goodbye") || 
            normalized.indexOf("see you") >= 0 || normalized == "night") {
            scoreFarewell += 5.0f;
        }

        // 4. Identity
        if (normalized.indexOf("who are you") >= 0 || normalized.indexOf("what are you") >= 0 || 
            normalized.indexOf("what is ran") >= 0 || normalized.indexOf("what's your name") >= 0 || 
            normalized.indexOf("your name") >= 0 || normalized.indexOf("who made you") >= 0 || 
            normalized.indexOf("who created you") >= 0) {
            scoreIdentity += 6.0f;
        }

        // 5. Capability
        if (normalized.indexOf("what can you do") >= 0 || normalized.indexOf("what are your capabilities") >= 0 || 
            normalized.indexOf("features") >= 0 || normalized.indexOf("what do you do") >= 0) {
            scoreCapability += 5.0f;
        }

        // 6. Memory Query
        if (normalized.indexOf("do you remember") >= 0 || normalized.indexOf("what do you remember") >= 0 || 
            normalized.indexOf("recall") >= 0 || normalized.indexOf("stored memories") >= 0 || 
            normalized.indexOf("what did i tell you") >= 0) {
            scoreMemQuery += 5.0f;
        }

        // 7. Project Query
        if (normalized.indexOf("charrmpass") >= 0 || normalized.indexOf("rfid") >= 0 || 
            normalized.indexOf("vehicle") >= 0 || normalized.indexOf("project") >= 0 || 
            normalized.indexOf("gate") >= 0 || normalized.indexOf("barrier") >= 0) {
            scoreProject += 5.0f;
        }

        // 8. Knowledge Query
        if (normalized.indexOf("pinout") >= 0 || normalized.indexOf("wiring") >= 0 || 
            normalized.indexOf("spi") >= 0 || normalized.indexOf("esp32") >= 0 || 
            normalized.indexOf("gpio") >= 0 || normalized.indexOf("cs pin") >= 0) {
            scoreKnowledge += 4.5f;
        }

        // 9. Status Query
        if (normalized.indexOf("status") >= 0 || normalized.indexOf("online") >= 0 || 
            normalized.indexOf("uptime") >= 0 || normalized.indexOf("free heap") >= 0 || 
            normalized.indexOf("ram") >= 0 || normalized.indexOf("battery") >= 0) {
            scoreStatus += 5.0f;
        }

        // 10. Time Query
        if (normalized.indexOf("time") >= 0 || normalized.indexOf("clock") >= 0 || normalized.indexOf("what time") >= 0) {
            scoreTime += 5.0f;
        }

        // 11. Help
        if (normalized == "help" || normalized.startsWith("help") || normalized.indexOf("commands") >= 0) {
            scoreHelp += 5.0f;
        }

        // 12. Thanks
        if (normalized.startsWith("thank") || normalized.indexOf("thanks") >= 0 || normalized.indexOf("appreciate") >= 0) {
            scoreThanks += 5.0f;
        }

        // Pick highest intent
        float highest = 0.0f;
        Intent best = INTENT_UNKNOWN;

        auto testScore = [&](Intent i, float s) {
            if (s > highest) {
                highest = s;
                best = i;
            }
        };

        testScore(INTENT_MEMORY_SAVE,     scoreMemSave);
        testScore(INTENT_MEMORY_DELETE,   scoreMemDelete);
        testScore(INTENT_IDENTITY,        scoreIdentity);
        testScore(INTENT_GREETING,        scoreGreeting);
        testScore(INTENT_FAREWELL,        scoreFarewell);
        testScore(INTENT_CAPABILITY,      scoreCapability);
        testScore(INTENT_MEMORY_QUERY,    scoreMemQuery);
        testScore(INTENT_PROJECT_QUERY,   scoreProject);
        testScore(INTENT_KNOWLEDGE_QUERY, scoreKnowledge);
        testScore(INTENT_STATUS_QUERY,    scoreStatus);
        testScore(INTENT_TIME_QUERY,      scoreTime);
        testScore(INTENT_HELP,            scoreHelp);
        testScore(INTENT_THANKS,          scoreThanks);

        confidence = highest / 6.0f; // Scale 0.0 - 1.0
        if (confidence < 0.40f) {
            return INTENT_UNKNOWN;
        }
        return best;
    }

    // ── Response Generation Pipeline ─────────────────────────
    AiResponse process(const String& rawInput) {
        String normalized = normalize(rawInput);
        String resolved   = contextManager.resolvePronoun(normalized);

        float confidence = 0.0f;
        Intent intent = detectIntent(resolved, confidence);

        AiResponse resp;
        resp.intent     = intent;
        resp.confidence = confidence;

        switch (intent) {
            case INTENT_GREETING: {
                static int gIdx = 0;
                const char* gGreetings[] = {
                    "Hello, Master. I'm here.",
                    "Hey, Master. What are we working on?",
                    "Hello! R.A.N. is online and ready."
                };
                resp.text = gGreetings[(gIdx++) % 3];
                resp.mood = "wave";
                contextManager.addExchange(rawInput, resp.text, intent, "GREETING");
                break;
            }

            case INTENT_FAREWELL: {
                resp.text = "Goodbye, Master. R.A.N. is standing by.";
                resp.mood = "idle";
                contextManager.addExchange(rawInput, resp.text, intent, "FAREWELL");
                break;
            }

            case INTENT_IDENTITY: {
                resp.text = "I'm R.A.N., the Raian AI Network.\nI am your personal offline AI companion running on your ESP32.";
                resp.mood = "happy";
                contextManager.addExchange(rawInput, resp.text, intent, "RAN");
                break;
            }

            case INTENT_CAPABILITY: {
                resp.text = "I can remember your project details, answer technical questions, search my local microSD knowledge, and talk with you completely offline.";
                resp.mood = "talking";
                contextManager.addExchange(rawInput, resp.text, intent, "CAPABILITY");
                break;
            }

            case INTENT_MEMORY_SAVE: {
                // Extract fact from "remember that ..." or "remember ..."
                String fact = normalized;
                if (fact.startsWith("remember that ")) {
                    fact = fact.substring(14);
                } else if (fact.startsWith("remember my ")) {
                    fact = fact.substring(12);
                } else if (fact.startsWith("remember ")) {
                    fact = fact.substring(9);
                }
                fact.trim();

                int id = memoryManager.saveMemory("NOTE", "user_note", fact);
                if (id > 0) {
                    resp.text = "Got it. I'll remember that " + fact + ".";
                    resp.mood = "happy";
                } else {
                    resp.text = "I couldn't write to microSD memory. Check card status.";
                    resp.mood = "shock";
                }
                contextManager.addExchange(rawInput, resp.text, intent, "MEMORY_SAVE");
                break;
            }

            case INTENT_MEMORY_QUERY:
            case INTENT_PROJECT_QUERY:
            case INTENT_KNOWLEDGE_QUERY: {
                float memScore = 0.0f;
                float knwScore = 0.0f;
                String memMatch = memoryManager.searchMemory(resolved, memScore);
                String knwMatch = memoryManager.searchKnowledge(resolved, knwScore);

                if (knwScore >= memScore && knwScore > 1.5f) {
                    resp.text = knwMatch;
                    resp.mood = "talking";
                    contextManager.addExchange(rawInput, resp.text, intent, "KNOWLEDGE");
                } else if (memScore > 1.5f) {
                    resp.text = "From your microSD memory:\n" + memMatch;
                    resp.mood = "wondering";
                    contextManager.addExchange(rawInput, resp.text, intent, "MEMORY");
                } else {
                    resp.text = "I don't have information about that in my local knowledge yet.";
                    resp.mood = "wondering";
                    contextManager.addExchange(rawInput, resp.text, intent, "UNKNOWN");
                }
                break;
            }

            case INTENT_STATUS_QUERY: {
                uint32_t freeHeap = ESP.getFreeHeap();
                uint32_t uptimeSec = millis() / 1000;
                resp.text = "R.A.N. STATUS:\n"
                            "• AI: LOCAL OFFLINE\n"
                            "• ESP32: OK\n"
                            "• microSD: " + String(sdStorage.ready ? "OK" : "MISSING") + "\n"
                            "• Free Heap: " + String(freeHeap / 1024) + " KB\n"
                            "• Memories: " + String(memoryManager.getCount()) + " records\n"
                            "• Uptime: " + String(uptimeSec) + "s";
                resp.mood = "happy";
                contextManager.addExchange(rawInput, resp.text, intent, "STATUS");
                break;
            }

            case INTENT_TIME_QUERY: {
                unsigned long s = millis() / 1000;
                unsigned long m = s / 60;
                unsigned long h = m / 60;
                char buf[32];
                snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h % 24, m % 60, s % 60);
                resp.text = "Current ESP32 uptime clock is " + String(buf) + ".";
                resp.mood = "idle";
                contextManager.addExchange(rawInput, resp.text, intent, "TIME");
                break;
            }

            case INTENT_HELP: {
                resp.text = "R.A.N. Commands:\n"
                            "• /status — Check hardware telemetry\n"
                            "• /memory — List stored records\n"
                            "• /remember <text> — Store fact in microSD\n"
                            "• /search <keyword> — Search memory\n"
                            "• /personality — View identity\n"
                            "• Or talk naturally with me offline!";
                resp.mood = "happy";
                contextManager.addExchange(rawInput, resp.text, intent, "HELP");
                break;
            }

            case INTENT_THANKS: {
                resp.text = "You're welcome, Master. Glad to help!";
                resp.mood = "happy";
                contextManager.addExchange(rawInput, resp.text, intent, "THANKS");
                break;
            }

            default:
            case INTENT_UNKNOWN: {
                // Check if any memory or knowledge matches even on unknown intent
                float memScore = 0.0f;
                float knwScore = 0.0f;
                String memMatch = memoryManager.searchMemory(resolved, memScore);
                String knwMatch = memoryManager.searchKnowledge(resolved, knwScore);

                if (knwScore > 2.0f) {
                    resp.text = knwMatch;
                    resp.mood = "talking";
                } else if (memScore > 2.0f) {
                    resp.text = "From your microSD memory:\n" + memMatch;
                    resp.mood = "wondering";
                } else {
                    resp.text = "I'm not sure what you mean. Can you say that another way?";
                    resp.mood = "wondering";
                }
                contextManager.addExchange(rawInput, resp.text, intent, "UNKNOWN");
                break;
            }
        }

        // Log to microSD conversations
        sdStorage.logExchange(rawInput, resp.text);
        return resp;
    }
} localAi;

// ============================================================
// 5. BLE GATT SERVER (Web Bluetooth HCI Bridge)
// ============================================================
BLEServer*         pServer            = nullptr;
BLECharacteristic* pTxChar            = nullptr;
BLECharacteristic* pRxChar            = nullptr;
bool               deviceConnected    = false;
bool               oldDeviceConnected = false;
char               bleRxBuffer[BLE_MAX_MSG_LEN];
size_t             bleRxLen           = 0;

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println(F("\n[BLE] >>> Web Browser Connected! <<<"));
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println(F("\n[BLE] >>> Web Browser Disconnected <<<"));
    }
};

class RxCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pChar) {
        uint8_t* data = pChar->getData();
        size_t len = pChar->getLength();

        for (size_t i = 0; i < len; i++) {
            char c = (char)data[i];
            if (c == BLE_TERMINATOR) {
                bleRxBuffer[bleRxLen] = '\0';
                String msg = String(bleRxBuffer);
                bleRxLen = 0;
                onBLEMessage(msg);
            } else if (bleRxLen < BLE_MAX_MSG_LEN - 1) {
                bleRxBuffer[bleRxLen++] = c;
            } else {
                bleRxLen = 0;
            }
        }
    }
};

bool initBLE() {
    BLEDevice::init(BLE_DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService* pService = pServer->createService(RAN_SERVICE_UUID);

    pTxChar = pService->createCharacteristic(
        RAN_TX_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxChar->addDescriptor(new BLE2902());

    pRxChar = pService->createCharacteristic(
        RAN_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pRxChar->setCallbacks(new RxCallbacks());

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(RAN_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMaxPreferred(0x0C);
    BLEDevice::startAdvertising();

    Serial.println(F("[BLE] GATT Server active. Name: RAN-ESP32"));
    return true;
}

void sendBLEMessage(const String& json) {
    if (!deviceConnected || !pTxChar) return;

    String payload = json + BLE_TERMINATOR;
    const uint8_t* data = (const uint8_t*)payload.c_str();
    size_t totalLen = payload.length();

    size_t offset = 0;
    while (offset < totalLen) {
        size_t chunkLen = (totalLen - offset < BLE_CHUNK_SIZE) ? (totalLen - offset) : BLE_CHUNK_SIZE;
        pTxChar->setValue(const_cast<uint8_t*>(data + offset), chunkLen);
        pTxChar->notify();
        offset += chunkLen;
        if (offset < totalLen) delay(15);
    }
}

void sendBLEResponse(const AiResponse& resp) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(1024);
#endif
    doc["type"]       = "response";
    doc["message"]    = resp.text;
    doc["mood"]       = resp.mood;
    doc["intent"]     = intentToString(resp.intent);
    doc["confidence"] = resp.confidence;

    String out;
    serializeJson(doc, out);
    sendBLEMessage(out);
}

void sendBLEStatus() {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(512);
#endif
    doc["type"]        = "status";
    doc["esp32"]       = true;
    doc["bluetooth"]   = deviceConnected;
    doc["sd"]          = sdStorage.ready;
    doc["ai"]          = true; // Local AI engine active
    doc["freeHeap"]    = ESP.getFreeHeap();
    doc["memoryCount"] = memoryManager.getCount();
    doc["uptime"]      = String(millis() / 1000) + "s";
    doc["deviceName"]  = BLE_DEVICE_NAME;

    String out;
    serializeJson(doc, out);
    sendBLEMessage(out);
}

void sendBLEMemoryList() {
    String raw = sdStorage.readFile("memory.json");
    if (raw.length() == 0) {
        raw = "{\"type\":\"memory_list\",\"memories\":[]}";
    } else {
        // Prepend type: memory_list if not present
        if (raw.indexOf("\"type\"") < 0) {
            raw.replace("{\"memories\"", "{\"type\":\"memory_list\",\"memories\"");
        }
    }
    sendBLEMessage(raw);
}

void sendBLEPersonality() {
    String content = sdStorage.readFile("personality.txt");
    if (content.length() == 0) {
        content = "Name: R.A.N.\nFull Name: Raian AI Network\nRole: Personal AI Companion";
    }
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(1024);
#endif
    doc["type"]    = "personality";
    doc["name"]    = "R.A.N.";
    doc["content"] = content;

    String out;
    serializeJson(doc, out);
    sendBLEMessage(out);
}

void sendBLEError(const String& code, const String& message) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(256);
#endif
    doc["type"]    = "error";
    doc["code"]    = code;
    doc["message"] = message;

    String out;
    serializeJson(doc, out);
    sendBLEMessage(out);
}

// ─── BLE Message Handler ──────────────────────────────────────
void onBLEMessage(const String& json) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(2048);
#endif
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        sendBLEError("PARSE_ERROR", "Invalid JSON format.");
        return;
    }

    String type = doc["type"].as<String>();

    if (type == "chat") {
        String msg = doc["message"].as<String>();
        AiResponse resp = localAi.process(msg);
        sendBLEResponse(resp);
    } else if (type == "command") {
        String cmd = doc["command"].as<String>();
        if (cmd == "GET_STATUS") sendBLEStatus();
        else if (cmd == "GET_PERSONALITY") sendBLEPersonality();
        else if (cmd == "GET_MEMORY") sendBLEMemoryList();
        else sendBLEError("UNKNOWN_CMD", "Unknown command: " + cmd);
    } else if (type == "memory_save") {
        String cat = doc["category"].as<String>();
        String key = doc["key"].as<String>();
        String val = doc["value"].as<String>();
        int id = memoryManager.saveMemory(cat, key, val);
        if (id > 0) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
            JsonDocument r;
#else
            DynamicJsonDocument r(128);
#endif
            r["type"] = "memory_saved";
            r["id"]   = id;
            String out;
            serializeJson(r, out);
            sendBLEMessage(out);
        } else {
            sendBLEError("SAVE_FAILED", "Failed to write memory to microSD.");
        }
    } else if (type == "memory_delete") {
        int id = doc["id"].as<int>();
        if (memoryManager.deleteMemory(id)) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
            JsonDocument r;
#else
            DynamicJsonDocument r(64);
#endif
            r["type"] = "memory_deleted";
            r["id"]   = id;
            String out;
            serializeJson(r, out);
            sendBLEMessage(out);
        } else {
            sendBLEError("DELETE_FAILED", "Memory #" + String(id) + " not found.");
        }
    }
}

// ============================================================
// 6. SERIAL DEBUG CONSOLE (115200 baud)
// ============================================================
String serialBuffer = "";
bool awaitingClearConfirm = false;

void processSerial() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            serialBuffer.trim();
            if (serialBuffer.length() > 0) {
                Serial.print(F("USER > "));
                Serial.println(serialBuffer);

                // Handle clear memory confirmation
                if (awaitingClearConfirm) {
                    if (serialBuffer == "CONFIRM CLEAR") {
                        memoryManager.clearMemory();
                        Serial.println(F("[MEMORY] All stored memories have been cleared."));
                    } else {
                        Serial.println(F("[MEMORY] Clear operation cancelled."));
                    }
                    awaitingClearConfirm = false;
                    serialBuffer = "";
                    return;
                }

                // Slash commands on Serial
                if (serialBuffer == "/clear_memory") {
                    Serial.println(F("\n[CAUTION] This will permanently erase R.A.N.'s memories!"));
                    Serial.println(F("Type: CONFIRM CLEAR to proceed."));
                    awaitingClearConfirm = true;
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer == "/help") {
                    Serial.println(F("\n=== R.A.N. Serial Commands ==="));
                    Serial.println(F("/status           - Hardware & AI status"));
                    Serial.println(F("/memory           - List all memories"));
                    Serial.println(F("/remember <text>  - Save fact to SD"));
                    Serial.println(F("/search <text>    - Search memory & knowledge"));
                    Serial.println(F("/forget <id>      - Delete memory by ID"));
                    Serial.println(F("/clear_memory     - Clear all memories (guarded)"));
                    Serial.println(F("/personality      - Display personality.txt"));
                    Serial.println(F("/knowledge        - Display knowledge.json"));
                    Serial.println(F("/device           - Display hardware specs"));
                    Serial.println(F("/storage          - Display microSD capacity"));
                    Serial.println(F("/reload           - Reload SD files"));
                    Serial.println(F("Or type any message to converse with R.A.N. directly!\n"));
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer == "/status") {
                    Serial.println(F("\n=================================="));
                    Serial.println(F("R.A.N. STATUS"));
                    Serial.println(F("=================================="));
                    Serial.println(F("AI Engine: LOCAL OFFLINE (ESP32)"));
                    Serial.print(F("microSD:   ")); Serial.println(sdStorage.ready ? F("OK") : F("FAILED"));
                    Serial.print(F("BLE:       ")); Serial.println(deviceConnected ? F("Connected") : F("Advertising"));
                    Serial.print(F("Memories:  ")); Serial.print(memoryManager.getCount()); Serial.println(F(" records"));
                    Serial.print(F("Free Heap: ")); Serial.print(ESP.getFreeHeap() / 1024); Serial.println(F(" KB"));
                    Serial.print(F("Uptime:    ")); Serial.print(millis() / 1000); Serial.println(F(" seconds\n"));
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer == "/memory") {
                    Serial.println(F("\n--- Stored Memories ---"));
                    Serial.println(sdStorage.readFile("memory.json"));
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer.startsWith("/remember ")) {
                    String fact = serialBuffer.substring(10);
                    fact.trim();
                    int id = memoryManager.saveMemory("NOTE", "serial_note", fact);
                    Serial.print(F("[MEMORY] Saved with ID #"));
                    Serial.println(id);
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer.startsWith("/search ")) {
                    String q = serialBuffer.substring(8);
                    float score = 0.0f;
                    String res = memoryManager.searchMemory(q, score);
                    Serial.print(F("[SEARCH] Result (Score ")); Serial.print(score); Serial.println(F("):"));
                    Serial.println(res.length() > 0 ? res : F("No matches found."));
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer.startsWith("/forget ")) {
                    int id = serialBuffer.substring(8).toInt();
                    if (memoryManager.deleteMemory(id)) {
                        Serial.println(F("[MEMORY] Deleted successfully."));
                    } else {
                        Serial.println(F("[MEMORY] ID not found."));
                    }
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer == "/personality") {
                    Serial.println(F("\n--- R.A.N. Personality ---"));
                    Serial.println(sdStorage.readFile("personality.txt"));
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer == "/knowledge") {
                    Serial.println(F("\n--- R.A.N. Knowledge Base ---"));
                    Serial.println(sdStorage.readFile("knowledge.json"));
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer == "/device") {
                    Serial.println(F("\n=== R.A.N. Device Specifications ==="));
                    Serial.println(F("Platform: ESP32-WROOM-32 (Xtensa dual-core @ 240MHz)"));
                    Serial.println(F("microSD:  SPI (CS=5, SCK=18, MOSI=23, MISO=19)"));
                    Serial.println(F("BLE:      GATT Server (RAN-ESP32)"));
                    Serial.println(F("AI Mode:  100% Offline Local Engine\n"));
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer == "/storage") {
                    if (sdStorage.ready) {
                        Serial.print(F("[STORAGE] microSD Total: "));
                        Serial.print((uint32_t)(SD.cardSize() / (1024 * 1024)));
                        Serial.println(F(" MB"));
                    } else {
                        Serial.println(F("[STORAGE] microSD not mounted."));
                    }
                    serialBuffer = "";
                    return;
                }

                if (serialBuffer == "/reload") {
                    sdStorage.begin();
                    Serial.println(F("[SYSTEM] Reloaded SD storage and databases."));
                    serialBuffer = "";
                    return;
                }

                // Natural language input -> feed to Local AI Engine!
                AiResponse resp = localAi.process(serialBuffer);
                Serial.print(F("R.A.N. ["));
                Serial.print(resp.mood);
                Serial.print(F(" | "));
                Serial.print(intentToString(resp.intent));
                Serial.println(F("] >"));
                Serial.println(resp.text);
                Serial.println();

                serialBuffer = "";
            }
        } else {
            serialBuffer += c;
        }
    }
}

// ============================================================
// 7. SETUP & MAIN LOOP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(400);

    Serial.println(F("\n===================================================="));
    Serial.println(F("   R.A.N. — RAIAN AI NETWORK (ESP32 Local AI)"));
    Serial.println(F("   Offline Personal Embedded Conversational Engine"));
    Serial.println(F("===================================================="));

    // 1. Initialize SD card
    Serial.print(F("[INIT] Mounting microSD Card... "));
    if (sdStorage.begin()) {
        Serial.println(F("OK"));
    } else {
        Serial.println(F("FAILED (Operating in transient local mode)"));
    }

    // 2. Initialize Memories & Knowledge
    Serial.print(F("[INIT] Local Memory Database... "));
    Serial.print(memoryManager.getCount());
    Serial.println(F(" records found."));

    // 3. Initialize BLE Server
    Serial.print(F("[INIT] Bluetooth Low Energy (RAN-ESP32)... "));
    if (initBLE()) {
        Serial.println(F("OK"));
    } else {
        Serial.println(F("FAILED"));
    }

    Serial.println(F("\n>>> R.A.N. READY! Type /help or chat below. <<<"));
    Serial.println(F(">>> Web HCI companion: http://localhost:5173/ <<<\n"));
}

void loop() {
    // BLE connection state management
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println(F("[BLE] Restarted advertising as RAN-ESP32."));
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    // Process serial debug console commands
    processSerial();

    // Periodic telemetry print every 30s
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 30000) {
        lastCheck = millis();
        Serial.print(F("[SYS] Free Heap: "));
        Serial.print(ESP.getFreeHeap() / 1024);
        Serial.print(F(" KB | BLE: "));
        Serial.println(deviceConnected ? F("Connected") : F("Advertising"));
    }

    delay(10);
}
