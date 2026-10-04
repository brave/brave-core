// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { gSafeBuiltins } from '//brave/ios/web/js_messaging/resources/safe_builtins.js'

// The message handler name to use with sendWebKitMessage when your JavaScript
// feature supports randomized message handler names.
export const messageHandlerName: string = 'gCrWebPlaceholderMessageHandlerName'

// A token to be used for validating communication with the browser
const messageHandlerToken: string = 'gCrWebPlaceholderMessageHandlerToken'

// Posts `message` to the webkit message handler specified by `handlerName` and
// embeds a token for the browser to validate
export function sendTokenizedWebKitMessage(
  handlerName: string,
  message: object | string,
) {
  gSafeBuiltins.sendWebKitMessage(handlerName, {
    token: messageHandlerToken,
    message: message,
  })
}

// Posts `message` to the webkit message handler specified by `handlerName` and
// embeds a token for the browser to validate and waits for a reply
export function sendTokenizedWebKitMessageWithReply(
  handlerName: string,
  message: object | string,
): Promise<any> {
  return gSafeBuiltins.sendWebKitMessageWithReply(handlerName, {
    token: messageHandlerToken,
    message: message,
  })
}

// Posts `message` to the browser via `window.prompt` with an embedded token for
// validation and waits for the reply synchronously. If the message isnt valid
// JSON or the response cannot be parsed as JSON then this returns null
export function sendTokenizedWebKitMessageSynchronously(
  handlerName: string,
  message: object | string,
): any | null {
  return gSafeBuiltins.sendWebKitMessageSynchronously(handlerName, {
    token: messageHandlerToken,
    message: message,
  })
}
