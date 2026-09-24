// Ambient declarations for Web Bluetooth API
declare interface Navigator {
  bluetooth: {
    requestDevice(options?: any): Promise<any>;
    getAvailability?(): Promise<boolean>;
  };
}

type BluetoothDevice = any;
type BluetoothRemoteGATTServer = any;
type BluetoothRemoteGATTCharacteristic = any;
type BluetoothRemoteGATTService = any;
