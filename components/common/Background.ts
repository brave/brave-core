// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const braveExtensionId = 'mnojpmjdmbbfmejpflffifhffcmidifd'

export namespace MessageTypes {
  export enum Today {
    getFeed = 'getFeed',
    getPublishers = 'getPublishers',
    indicatingOpen = 'indicatingOpen',
    getImageData = 'getImageData',
  }
}
export type Payload = any
type MessageName = string | number
export type Action = {
  messageType: MessageName
  payload: Payload
}
export type SendResponseFunction<T> = (responsePayload: T)
  => void
export type HandlerFunction<T, U> = (payload: T, sender: any, sendResponse: SendResponseFunction<U>)
  => void
export type NHandlerFunction<U> = (payload: void, sender: any, sendResponse: SendResponseFunction<U>)
  => void

// Client-side scripts call this to send a message to the background
export function send<U=void, T=void>(messageType: string, payload?: T): Promise<U> {
  // TODO: verify comms channel isn't closed prematurely first. If so, wait and try again.
  console.debug(`Sending data to brave extension for ${messageType}`, { messageType, payload })
  return new Promise(function (resolve) {
    chrome.runtime.sendMessage(braveExtensionId, { messageType, payload }, function (responseData: U) {
      console.debug(`got response from brave extension for "${messageType}"`, responseData)
      resolve(responseData)
    })
  })
}

let isListening = false
let messageHandlers: Map<MessageName, HandlerFunction<any, any>>

// Background scripts call this to set up listeners
export function setListener<U, T=void>(messageType: MessageName, handler: HandlerFunction<T, U>): void {
  if (!messageHandlers) {
    messageHandlers = new Map()
  }
  messageHandlers.set(messageType, handler)
  if (!isListening && chrome && chrome.runtime && chrome.runtime.onMessageExternal) {
    isListening = true
    chrome.runtime.onMessageExternal.addListener(onMessageExternal)
  }
}

function onMessageExternal<T> (req: Action, sender: any, sendResponse: SendResponseFunction<T>) {
  console.log('onMessageExternal', req, sender)
  // TODO: check sender
  // validate
  if (!req.messageType) {
    return
  }
  const handler = messageHandlers.get(req.messageType)
  if (handler) {
    handler(req.payload, sender, sendResponse)
    return true
  } else {
    console.error('No handler for incoming external message', { req, sender })
  }
  return false
}