// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWebKitMessage, sendWebKitMessageWithReply} from
  '//ios/web/public/js_messaging/resources/utils.js';

function getSecurityToken() {
  return '{{PlaceholderSecurityToken}}';
}

export function sendSecureWebKitMessage(handlerName: string, message: object|string) {
  const body = {
    'code': getSecurityToken(),
    'message': message
  };
  sendWebKitMessage(handlerName, body);
}

export function sendSecureWebKitMessageWithReply(
    handlerName: string, message: object|string): Promise<any> {
  const body = {
    'code': getSecurityToken(),
    'message': message
  };
  return sendWebKitMessageWithReply(handlerName, body);
};
