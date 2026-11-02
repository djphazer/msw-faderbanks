import {
  deviceForId,
  toSysexArray,
} from "$lib/configuration";
import type { ControllerConfiguration } from "$lib/types";
import type { Output } from "webmidi";

const SYSEX_CONSTANTS = {
  sysexMfgId: [0x7d, 0x00, 0x00],
  requestInfoMsg: 0x1f,
  updateConfigMsg: 0x0e,

  toBootloader: 0x0b,
  factoryResetMsg: 0x1a // 1ntiAlize
};

export const isOxionSysex = (data: number[]) => {
  return (
    data[1] == SYSEX_CONSTANTS.sysexMfgId[0] &&
    data[2] == SYSEX_CONSTANTS.sysexMfgId[1] &&
    data[3] == SYSEX_CONSTANTS.sysexMfgId[2]
  );
};

export const sendConfiguration = (
  configuration: ControllerConfiguration,
  output: Output
) => {
    sendFullConfiguration(configuration, output);
};

export const sendFullConfiguration = (
  configuration: ControllerConfiguration,
  output: Output
) => {
  output.sendSysex(SYSEX_CONSTANTS.sysexMfgId, [
    SYSEX_CONSTANTS.updateConfigMsg,
    ...toSysexArray(configuration)
  ]);
};

export const requestConfig = (output: Output) => {
  output.sendSysex(SYSEX_CONSTANTS.sysexMfgId, [
    SYSEX_CONSTANTS.requestInfoMsg
  ]);
};

export const sendFactoryResetRequest = (output: Output) => {
  output.sendSysex(SYSEX_CONSTANTS.sysexMfgId, [
    SYSEX_CONSTANTS.factoryResetMsg
  ]);
};

export const sendToBootloaderRequest = (output: Output) => {
  output.sendSysex(SYSEX_CONSTANTS.sysexMfgId, [
    SYSEX_CONSTANTS.toBootloader
  ]);
};
