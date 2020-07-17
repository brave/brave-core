const braveExtensionId = 'mnojpmjdmbbfmejpflffifhffcmidifd'

export type Action = {
  messageType: MessageType
  payload: Payload
}
export type SendResponseFunction = (responsePayload: GetFeedResponse | void)
  => void
export type HandlerFunction<T> = (payload: T, sender: any, sendResponse: SendResponseFunction)
  => void

export enum MessageType {
  getFeed = 1,
  indicatingOpen = 2
}
export type GetFeedPayload = {
  hi: number
}

export type GetFeedResponse = {
  feed: {} | BraveToday.Feed
}

export type Payload = GetFeedPayload
export type Response = GetFeedResponse

// Client-side scripts call this to send a message to the background
export function send<T = Payload, U = Response>(messageType: MessageType, payload?: T): Promise<U> {
  console.debug(`Sending data to brave extension for ${messageType}`, { messageType, payload })
  return new Promise(function (resolve) {
    chrome.runtime.sendMessage(braveExtensionId, { messageType, payload }, function (responseData: U) {
      console.debug(`got response from brave extension for "${messageType}"`, responseData)
      resolve(responseData)
    })
  })
}

let isListening = false
let messageHandlers: Map<MessageType, HandlerFunction<any>>

// Background scripts call this to set up listeners
export function setListener<T = Payload>(messageType: MessageType, handler: HandlerFunction<T>): void {
  if (!messageHandlers) {
    messageHandlers = new Map()
  }
  messageHandlers.set(messageType, handler)
  if (!isListening && chrome && chrome.runtime && chrome.runtime.onMessageExternal) {
    isListening = true
    chrome.runtime.onMessageExternal.addListener(onMessageExternal)
  }
}

function onMessageExternal (req: Action, sender: any, sendResponse: SendResponseFunction) {
  console.log('onMessageExternal', req, sender)
  // TODO: check sender
  // validate
  if (!req.messageType) {
    return
  }
  const handler = messageHandlers.get(req.messageType)
  if (handler) {
    handler(req.payload, sender, sendResponse)
  } else {
    console.error('No handler for incoming external message', { req, sender })
  }
}