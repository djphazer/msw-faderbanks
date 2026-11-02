export type Control = {
  channel: number;
  cc: number;
  val?: number;
};

export type ControllerConfiguration = {
  highResolution: boolean;
  ledOn: boolean;
  ledFlash: boolean;
  controllerFlip: boolean;
  midiThru: boolean;
  deviceId: number;
  i2cLeader: boolean;
  faderMin: number;
  faderMax: number;
  bootDelay: number;
  i2cSlaveAddress: string;
  firmwareVersion: string;
  usbControls: Control[];
  trsControls: Control[];
};
