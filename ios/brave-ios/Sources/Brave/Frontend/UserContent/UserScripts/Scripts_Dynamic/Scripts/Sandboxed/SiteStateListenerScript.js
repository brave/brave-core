// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.execute(function($) {
  (function() {
    'use strict'

    const messageHandler = '$<message_handler>';
    const sendMessage = $((data) => {
      return $.postNativeMessage(messageHandler, {
        "securityToken": SECURITY_TOKEN,
        "data": data
      });
    });

    sendMessage({
      "windowURL": window.location.href,
      "windowOriginURL": window.origin
    });
  })();
});
