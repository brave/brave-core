import { loadTimeData } from '../../../common/loadTimeData'
import { Unsuccessful } from 'trezor-connect'

export const kTrezorBridgeUrl = loadTimeData.getString('braveWalletTrezorBridgeUrl')

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

// Trezor library is loaded inside the chrome-untrusted webui page
// and communication is going through posting messages between parent window
// and frame window. This class handles low level messages transport to add,
// remove callbacks and allows to process messages for childrens.
abstract class MessagingTransport {
  constructor () {
    this.handlers = new Map<string, Function>()
  }

  handlers: Map<string, Function>

  addCommandHandler = (id: string, listener: Function): Boolean => {
    if (!this.handlers.size) {
      this.addWindowMessageListener()
      this.handlers.clear()
    }
    if (this.handlers.has(id)) {
      return false
    }
    this.handlers.set(id, listener)
    return true
  }

  removeCommandHandler = (id: string) => {
    if (!this.handlers.has(id)) {
      return false
    }
    this.handlers.delete(id)
    if (!this.handlers.size) {
      this.removeWindowMessageListener()
    }
    return true
  }

  abstract onMessageReceived (event: MessageEvent): unknown

  private addWindowMessageListener = () => {
    window.addEventListener('message', this.onMessageReceived)
  }

  private removeWindowMessageListener = () => {
    window.removeEventListener('message', this.onMessageReceived)
  }
}

// Handles sending messages to the Trezor library, creates untrusted iframe,
// loads library and allows to send commans to the library and subscribe
// for responses.
export class TrezorBridgeTransport extends MessagingTransport {
  constructor (bridgeFrameUrl: string) {
    super()
    this.bridgeFrameUrl = bridgeFrameUrl
    // @ts-ignore
    this.frameId = crypto.randomUUID()
  }

  frameId: string
  bridgeFrameUrl: string

  sendCommandToTrezorFrame = async (command: TrezorFrameCommand, listener: Function): Promise<Boolean> => {
    let bridge = this.getBridge() as HTMLIFrameElement
    if (!bridge) {
      bridge = await this.createBridge() as HTMLIFrameElement
    }
    if (!bridge.contentWindow) {
      return false
    }
    this.addCommandHandler(command.id, listener)

    bridge.contentWindow.postMessage(command, this.bridgeFrameUrl)
    return true
  }

  onMessageReceived = (event: MessageEvent) => {
    if (event.origin !== this.getTrezorBridgeOrigin() ||
        event.type !== 'message' ||
        !this.handlers.size) {
      return
    }

    const message = event.data as TrezorFrameCommand
    if (!message || !this.handlers.has(message.id)) {
      return
    }
    const callback = this.handlers.get(message.id) as Function
    callback.call(this, message)
    this.removeCommandHandler(event.data.id)
  }

  private getTrezorBridgeOrigin = () => {
    return (new URL(this.bridgeFrameUrl)).origin
  }

  private createBridge = () => {
    return new Promise((resolve) => {
      let element = document.createElement('iframe')
      element.id = this.frameId
      element.src = this.bridgeFrameUrl
      element.style.display = 'none'
      element.onload = () => {
        resolve(element)
      }
      document.body.appendChild(element)
    })
  }

  private getBridge = () => {
    return document.getElementById(this.frameId)
  }
}

// Handles commands forwarding to the Trezor library inside the iframe.
export class TrezorCommandHandler extends MessagingTransport {
  onMessageReceived = async (event: MessageEvent) => {
    if (event.origin !== event.data.origin || event.type !== 'message' || !event.source) {
      return
    }
    const message = event.data as TrezorFrameCommand
    if (!message || !this.handlers.has(message.command)) {
      return
    }
    const callback = this.handlers.get(message.command) as Function
    const response = await callback.call(this, event.data)
    const target = event.source as Window
    target.postMessage(response, response.origin)
  }
}

let handler: TrezorCommandHandler | undefined = undefined

export function addTrezorCommandHandler (command: TrezorCommand, listener: Function): Boolean {
  if (!handler) {
    handler = new TrezorCommandHandler()
  }
  return handler.addCommandHandler(command, listener)
}

let transport: TrezorBridgeTransport | undefined = undefined

export async function sendTrezorCommand (command: TrezorFrameCommand, listener: Function): Promise<Boolean> {
  if (!transport) {
    transport = new TrezorBridgeTransport(kTrezorBridgeUrl)
  }
  return transport.sendCommandToTrezorFrame(command, listener)
}
