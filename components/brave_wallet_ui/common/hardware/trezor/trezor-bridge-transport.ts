// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { kTrezorBridgeUrl, MessagingTransport, TrezorErrorsCodes, TrezorFrameCommand } from './trezor-messages'

// Handles sending messages to the Trezor library, creates untrusted iframe,
// loads library and allows to send commands to the library and subscribe
// for responses.
export class TrezorBridgeTransport extends MessagingTransport {
  constructor (bridgeFrameUrl: string) {
    super()
    this.bridgeFrameUrl = bridgeFrameUrl
    this.frameId = crypto.randomUUID()
  }

  private readonly frameId: string
  private readonly bridgeFrameUrl: string
  private bridge?: HTMLIFrameElement

  closeBridge = () => {
    if (!this.bridge || !this.hasBridgeCreated()) {
      return
    }
    const element = document.getElementById(this.frameId)
    element?.parentNode?.removeChild(element)
  }

  // T is response type, e.g. UnlockResponse. Resolves as `false` if transport error
  sendCommandToTrezorFrame = <T> (command: TrezorFrameCommand): Promise<T | TrezorErrorsCodes> => {
    return new Promise<T | TrezorErrorsCodes>(async (resolve) => {
      if (!this.bridge && !this.hasBridgeCreated()) {
        this.bridge = await this.createBridge()
      }
      if (!this.bridge || !this.bridge.contentWindow) {
        resolve(TrezorErrorsCodes.BridgeNotReady)
        return
      }
      if (!this.addCommandHandler(command.id, resolve)) {
        resolve(TrezorErrorsCodes.CommandInProgress)
        return
      }
      this.bridge.contentWindow.postMessage(command, this.bridgeFrameUrl)
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

  private readonly getTrezorBridgeOrigin = () => {
    return (new URL(this.bridgeFrameUrl)).origin
  }

  private readonly createBridge = () => {
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

  private readonly hasBridgeCreated = (): boolean => {
    return document.getElementById(this.frameId) !== null
  }
}

let transport: TrezorBridgeTransport
export async function sendTrezorCommand<T> (command: TrezorFrameCommand): Promise<T | TrezorErrorsCodes> {
  if (!transport) {
    transport = new TrezorBridgeTransport(kTrezorBridgeUrl)
  }
  return transport.sendCommandToTrezorFrame<T>(command)
}

export async function closeTrezorBridge () {
  if (!transport) {
    return
  }
  return transport.closeBridge()
}
