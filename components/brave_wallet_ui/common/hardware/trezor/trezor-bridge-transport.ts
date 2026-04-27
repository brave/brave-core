// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  MessagingTransport,
  TrezorErrorsCodes,
  TrezorFrameCommand,
} from './trezor-messages'

// Handles sending messages to the Trezor library iframe and subscribing for
// responses.
export class TrezorBridgeTransport extends MessagingTransport {
  constructor(bridgeFrameUrl: string, bridge: HTMLIFrameElement) {
    super()
    this.bridgeFrameUrl = bridgeFrameUrl
    this.bridge = bridge
  }

  private readonly bridgeFrameUrl: string
  private readonly bridge: HTMLIFrameElement

  closeBridge = () => {
    if (!this.bridge) {
      return
    }
    this.bridge.parentNode?.removeChild(this.bridge)
  }

  /**
   * `T` is response type, e.g. UnlockResponse. Resolves as `false` if transport
   * error
   */
  sendCommandToTrezorFrame = <T>(
    command: TrezorFrameCommand,
  ): Promise<T | TrezorErrorsCodes> => {
    return new Promise<T | TrezorErrorsCodes>((resolve) => {
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
    if (
      event.origin !== this.getTrezorBridgeOrigin()
      || event.type !== 'message'
      || !this.handlers.size
    ) {
      return
    }

    const message = event.data as TrezorFrameCommand
    if (!message || !this.handlers.has(message.id)) {
      return
    }
    const callback = this.handlers.get(message.id)!
    callback.call(this, message)
    this.removeCommandHandler(event.data.id)
  }

  private readonly getTrezorBridgeOrigin = () => {
    return new URL(this.bridgeFrameUrl).origin
  }
}

export async function createTrezorBridge(
  bridgeFrameUrl: string,
): Promise<HTMLIFrameElement> {
  let element = document.createElement('iframe')
  element.id = crypto.randomUUID()
  element.style.display = 'none'

  await new Promise<void>((resolve, reject) => {
    element.onload = () => {
      resolve()
    }
    element.onerror = () => {
      reject(
        new Error(`Failed to load Trezor bridge iframe: ${bridgeFrameUrl}`),
      )
    }

    document.body.appendChild(element)
    element.src = bridgeFrameUrl
  })

  return element
}

export async function closeTrezorBridge(transport: TrezorBridgeTransport) {
  return transport.closeBridge()
}
