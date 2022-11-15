// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { MessagingTransport, TrezorFrameCommand, TrezorCommand } from './trezor-messages'

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

export function addTrezorCommandHandler (command: TrezorCommand, listener: Function): Boolean {
  if (!handler) {
    handler = new TrezorCommandHandler()
  }
  return handler.addCommandHandler(command, listener)
}
