// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { gSafeBuiltins }
  from "//brave/ios/web/js_messaging/resources/safe_builtins.js";

const token = 'gCrWebPlaceholderMessageHandlerToken';

export function sendTokenizedWebKitMessage(
    handlerName: string, message: object|string) {
  gSafeBuiltins.sendWebKitMessage(handlerName, {
    token: token,
    message: message
  });
}

export function sendTokenizedWebKitMessageWithReply(
    handlerName: string, message: object|string): Promise<any> {
  return gSafeBuiltins.sendWebKitMessageWithReply(handlerName, {
    token: token,
    message: message
  })
}

export function sendTokenizedWebKitMessageSynchronously(
  handlerName: string, message: object|string) {
  return gSafeBuiltins.sendWebKitMessageSynchronously(handlerName, {
    token: token,
    message: message
  })
}

// The message handler name to use with sendWebKitMessage when your JavaScript
// feature supports randomized message handler names.
export const messageHandlerName: string = 'gCrWebPlaceholderMessageHandlerName';
