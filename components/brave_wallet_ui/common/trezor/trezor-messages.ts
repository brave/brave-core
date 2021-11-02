import { Unsuccessful } from 'trezor-connect'
import { loadTimeData } from '../../../common/loadTimeData'
export const kTrezorBridgeFrameId = 'trezor-untrusted-frame'
export const kTrezorBridgeUrl = loadTimeData.getString('braveWalletTrezorBridgeUrl')

export enum TrezorCommand {
  Unlock = 'trezor-unlock',
  GetAccounts = 'trezor-get-accounts'
}
export type CommandMessage = {
  command: TrezorCommand
  id: number
  origin: string
}
export type TrezorAccountPath = {
  path: string
}
export type GetAccountsCommand = CommandMessage & {
  command: TrezorCommand.GetAccounts,
  paths: TrezorAccountPath[]
}
export type UnlockCommand = CommandMessage & {
  command: TrezorCommand.Unlock
}
export type UnlockResponse = CommandMessage & {
  result: Boolean,
  error?: Unsuccessful
}
export type TrezorAccount = {
  publicKey: string
  serializedPath: string,
  fingerprint: number
}
export type TrezorError = {
  error: string,
  code: string
}
export type TrezorGetAccountsResponse = {
  success: Boolean
  payload: TrezorAccount[] | TrezorError
}
export type GetAccountsResponsePayload = CommandMessage & {
  payload: TrezorGetAccountsResponse
}
export type TrezorFrameCommand = GetAccountsCommand | UnlockCommand
export type TrezorFrameResponse = UnlockResponse | GetAccountsResponsePayload
export function postResponseToWallet (target: Window, message: TrezorFrameResponse) {
  target.postMessage(message, message.origin)
}

export function postToTrezorFrame (target: Window, message: TrezorFrameCommand) {
  target.postMessage(message, kTrezorBridgeUrl)
}
