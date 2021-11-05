// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { kTrezorBridgeUrl, MessagingTransport, TrezorFrameCommand } from './trezor-messages'

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

  // T is response type, e.g. UnlockResponse. Resolves as `false` if transport error
  sendCommandToTrezorFrame = <T> (command: TrezorFrameCommand): Promise<T | false> => {
    return new Promise<T>(async (resolve) => {
      let bridge = this.getBridge()
      if (!bridge) {
        bridge = await this.createBridge()
      }
      if (!bridge.contentWindow) {
        return Promise.resolve(false)
      }
      this.addCommandHandler(command.id, resolve)
      bridge.contentWindow.postMessage(command, this.bridgeFrameUrl)
      return
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

let transport: TrezorBridgeTransport
export async function sendTrezorCommand<T> (command: TrezorFrameCommand): Promise<T | false> {
  if (!transport) {
    transport = new TrezorBridgeTransport(kTrezorBridgeUrl)
  }
  return transport.sendCommandToTrezorFrame<T>(command)
}
