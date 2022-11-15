/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  LedgerBridgeErrorCodes,
  LedgerFrameCommand,
  LedgerFrameResponse,
  LedgerCommandHandlerUnion
} from './ledger-messages'

// LedgerMessagingTransport is a generic bi-directional messaging utility for
// Window objects. It supports supports both (1) the sending of messages via postMessage
// to a window object at a targetUrl and subscribing of responses, and (2)
// the definition of handlers to be run when a different LedgerMessagingTransport
// instance at a different Window sends messages to it.
export class LedgerMessagingTransport {
  protected targetWindow: Window
  protected targetUrl: string
  protected handlers: Map<string, Function>

  constructor (targetWindow: Window, targetUrl: string) {
    this.targetWindow = targetWindow
    this.targetUrl = targetUrl
    this.handlers = new Map<string, LedgerCommandHandlerUnion<LedgerFrameResponse>>()
  }

  // T is response type, e.g. GetAccountResponse
  sendCommand = <T> (command: LedgerFrameCommand): Promise<T | LedgerBridgeErrorCodes> => {
    return new Promise<T | LedgerBridgeErrorCodes>(async (resolve) => {
      // Set handler for the response by passing the resolve function to be run
      // when targetWindow responds using the same command.id.
      // This allows us to simply `await` the sendCommand response
      if (!this.addCommandHandler<T>(command.id, resolve)) {
        resolve(LedgerBridgeErrorCodes.CommandInProgress)
        return
      }
      this.targetWindow.postMessage(command, this.targetUrl)
    })
  }

  // addCommandHandler registers a callback function to be run when a the
  // Window receives a command with the given ID. Command handlers cannot
  // be reassigned until removed.
  protected addCommandHandler = <T>(
    id: string,
    handler: LedgerCommandHandlerUnion<T>
  ): boolean => {
    if (!this.handlers.size) {
      this.addWindowMessageListener()
    }
    if (this.handlers.has(id)) {
      return false
    }
    this.handlers.set(id, handler)
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

  // onMessageReceived processes message events received by the Window.
  // It fetches and runs the handler if one exists. If the received message event
  // itself is a response to a sendCommand, the command handler is then removed.
  // Otherwise, the response is posted back to the Window that sent the message.
  protected onMessageReceived = async (event: MessageEvent<LedgerFrameCommand>) => {
    if (event.type !== 'message' || (event.origin !== this.targetUrl) || !event.source) {
      return
    }

    const message = event.data
    if (!message || !this.handlers.has(message.command)) {
      return
    }
    const callback = this.handlers.get(message.command)
    if (!(typeof callback === 'function')) {
      return
    }
    const response = await callback(event.data)
    const isResponseMessage = (event.origin !== event.data.origin)
    if (isResponseMessage) {
      this.removeCommandHandler(event.data.id)
      return
    }
    if (!response) {
      return
    }
    this.targetWindow.postMessage(response, response.origin)
  }

  private readonly addWindowMessageListener = () => {
    window.addEventListener('message', this.onMessageReceived)
  }

  private readonly removeWindowMessageListener = () => {
    window.removeEventListener('message', this.onMessageReceived)
  }
}
