// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const braveExtensionId = 'mnojpmjdmbbfmejpflffifhffcmidifd'

export namespace MessageTypes {

  export enum Today {
    getFeed = 'getFeed',
    getPublishers = 'getPublishers',
    indicatingOpen = 'indicatingOpen',
    getImageData = 'getImageData',
    setPublisherPref = 'setPublisherPref',
    isFeedUpdateAvailable = 'isFeedUpdateAvailable',
    resetPrefsToDefault = 'resetPrefsToDefault'
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

function isAllowedMessageSender (sender: chrome.runtime.MessageSender): boolean {
  if (sender.origin && sender.origin.startsWith('chrome://')) {
    return true
  }
  return false
}

// Client-side scripts call this to send a message to the background
export function send<U= void, T= void> (messageType: string, payload?: T): Promise<U> {
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
export function setListener<U = {}, T= void> (messageType: MessageName, handler: HandlerFunction<T, U>): void {
  if (!messageHandlers) {
    messageHandlers = new Map()
  }
  messageHandlers.set(messageType, handler)
  if (!isListening && chrome && chrome.runtime && chrome.runtime.onMessageExternal) {
    isListening = true
    chrome.runtime.onMessageExternal.addListener(onMessageExternal)
  }
}

function onMessageExternal<T> (req: Action, sender: chrome.runtime.MessageSender, sendResponse: SendResponseFunction<T>) {
  // Check Permissions
  if (!isAllowedMessageSender(sender)) {
    console.warn(`Received external message from a sender who is not allowed to send messages to this background. Origin: ${sender.origin}, URL: ${sender.url}`)
    return
  }
  // validate
  if (!req.messageType) {
    console.warn(`Unknown external message sent to Background from url ${sender.url}. No messageType property specified.`)
    return
  }
  console.debug(`Background received external message from ${sender.origin}: ${req.messageType}.`, sender.url)
  const handler = messageHandlers.get(req.messageType)
  if (!handler) {
    console.error(`No handler for received external message '${req.messageType}' received from ${sender.origin}`)
    return false
  }
  handler(req.payload, sender, sendResponse)
  return true
}
