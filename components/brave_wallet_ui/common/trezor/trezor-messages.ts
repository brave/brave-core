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

  protected handlers: Map<string, Function>

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

  protected removeCommandHandler = (id: string) => {
    if (!this.handlers.has(id)) {
      return false
    }
    this.handlers.delete(id)
    if (!this.handlers.size) {
      this.removeWindowMessageListener()
    }
    return true
  }

  protected abstract onMessageReceived (event: MessageEvent): unknown

  private addWindowMessageListener = () => {
    window.addEventListener('message', this.onMessageReceived)
  }

  private removeWindowMessageListener = () => {
    window.removeEventListener('message', this.onMessageReceived)
  }
}

// Handles sending messages to the Trezor library, creates untrusted iframe,
// loads library and allows to send commands to the library and subscribe
// for responses.
export class TrezorBridgeTransport extends MessagingTransport {
  constructor (bridgeFrameUrl: string) {
    super()
    this.bridgeFrameUrl = bridgeFrameUrl
    // @ts-ignore
    this.frameId = crypto.randomUUID()
  }

  private frameId: string
  private bridgeFrameUrl: string

  // T is response type, e.g. UnlockResponse. Resolves as `false` if transport errro
  sendCommandToTrezorFrame = async<T> (command: TrezorFrameCommand): Promise<T | false> => {
    return new Promise<T>(async (resolve) => {
      let bridge = this.getBridge()
      if (!bridge) {
        bridge = await this.createBridge()
      }
      if (!bridge.contentWindow) {
        Promise.resolve(false)
        return
      }
      this.addCommandHandler(command.id, resolve)
      bridge.contentWindow.postMessage(command, this.bridgeFrameUrl)
    })
  }

  protected onMessageReceived = (event: MessageEvent) => {
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
    return new Promise<HTMLIFrameElement>((resolve) => {
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

  private getBridge = (): HTMLIFrameElement | null => {
    return document.getElementById(this.frameId) as HTMLIFrameElement | null
  }
}

// Handles commands forwarding to the Trezor library inside the iframe.
export class TrezorCommandHandler extends MessagingTransport {
  protected onMessageReceived = async (event: MessageEvent) => {
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

let handler: TrezorCommandHandler
let transport: TrezorBridgeTransport

export function addTrezorCommandHandler (command: TrezorCommand, listener: Function): Boolean {
  if (!handler) {
    handler = new TrezorCommandHandler()
  }
  return handler.addCommandHandler(command, listener)
}

export async function sendTrezorCommand<T> (command: TrezorFrameCommand): Promise<T | false> {
  if (!transport) {
    transport = new TrezorBridgeTransport(kTrezorBridgeUrl)
  }
  return transport.sendCommandToTrezorFrame<T>(command)
}

// Caller example:
// async function doSomething () {
//   const transport = new TrezorBridgeTransport()
//   const data = await transport.sendTrezorCommand<UnlockResponse>({command: TrezorCommand.Unlock, id: '1', origin: 'a'})
//   if (data == false) {
//     // error
//     return
//   }
//   // ts knows data is UnlockResponse
//   data
// }
