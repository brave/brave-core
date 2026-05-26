// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  sendTokenizedWebKitMessage,
  // sendTokenizedWebKitMessageSynchronously,
  messageHandlerName
} from "//brave/ios/web/js_messaging/resources/utils.js";

setInterval(() => {
  const o = {"ThisIsAKey": "test"};
  sendTokenizedWebKitMessage(messageHandlerName, o);
}, 2500);

// setTimeout(() => {
//   const o = {"ThisIsAKey": "test"};
//   sendTokenizedWebKitMessageSynchronously(messageHandlerName, o);
// }, 2500);
