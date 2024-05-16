// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

"use strict";

window.__firefox__.execute(function($) {
  const messageHandler = '$<message_handler>';

  const sendMessage = $((callback, timeout) => {
    let currentTimeout = window.setTimeout(() => {
      delete currentTimeout
      callback()
    }, timeout)
    
    $.postNativeMessage(messageHandler, {
      "securityToken": SECURITY_TOKEN
    }).then(() => {
      if (currentTimeout === undefined) { return }
      window.clearTimeout(currentTimeout)
      delete currentTimeout
      callback()
    });
  });

  window.requestIdleCallback = $(function(callback, options) {
    if (options !== undefined && options.timeout !== undefined) {
      sendMessage(callback, options.timeout)
    } else {
      sendMessage(callback, 5000)
    }
  }, /*overrideToString=*/false);
});
