import { Unsuccessful } from 'trezor-connect'
import { loadTimeData } from '../../../common/loadTimeData'
export const kTrezorBridgeFrameId = 'trezor-untrusted-frame'
export const kTrezorBridgeUrl = loadTimeData.getString('braveWalletTrezorBridgeUrl')
import { getLocale } from '../../../common/locale'

export enum TrezorCommand {
  Unlock = 'trezor-unlock',
  GetAccounts = 'trezor-get-accounts'
}
export type CommandMessage = {
  command: TrezorCommand
  id: string
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

export class TrezorBridgeTransport {
  constructor () {
    this.pendingRequests = new Map<string, Function>()
  }

  pendingRequests: Map<string, Function>

  postMessage = async (command: TrezorFrameCommand) => {
    let bridge = this.getBridge() as HTMLIFrameElement
    if (!bridge) {
      bridge = await this.createBridge() as HTMLIFrameElement
    }
    if (!bridge.contentWindow) {
      throw Error(getLocale('braveWalletCreateBridgeError'))
    }
    postToTrezorFrame(bridge.contentWindow, command)
  }

  private addWindowMessageListener = () => {
    window.addEventListener('message', this.onMessageReceived)
  }

  private removeWindowMessageListener = () => {
    window.removeEventListener('message', this.onMessageReceived)
  }
  private getTrezorBridgeOrigin = () => {
    return (new URL(kTrezorBridgeUrl)).origin
  }

  private createBridge = () => {
    return new Promise((resolve) => {
      let element = document.createElement('iframe')
      element.id = kTrezorBridgeFrameId
      element.src = kTrezorBridgeUrl
      element.style.display = 'none'
      element.onload = () => {
        resolve(element)
      }
      document.body.appendChild(element)
    })
  }
  addEventListener = (id: string, listener: Function) => {
    if (!this.pendingRequests.size) {
      this.addWindowMessageListener()
      this.pendingRequests.clear()
    }
    if (this.pendingRequests.has(id)) {
      return false
    }
    this.pendingRequests.set(id, listener)
    return true
  }

  private removeEventListener = (id: string) => {
    if (!this.pendingRequests.has(id)) {
      return false
    }
    this.pendingRequests.delete(id)
    if (!this.pendingRequests.size) {
      this.removeWindowMessageListener()
    }
    return true
  }

  private getBridge = () => {
    return document.getElementById(kTrezorBridgeFrameId)
  }

  private onMessageReceived = (event: MessageEvent) => {
    if (event.origin !== this.getTrezorBridgeOrigin() ||
        event.type !== 'message' ||
        !this.pendingRequests.size) {
      return
    }
    const message = event.data as TrezorFrameCommand
    if (!message || !this.pendingRequests.has(message.id)) {
      return
    }
    const callback = this.pendingRequests.get(message.id) as Function
    callback.call(this, message)
    this.removeEventListener(event.data.id)
  }
}
