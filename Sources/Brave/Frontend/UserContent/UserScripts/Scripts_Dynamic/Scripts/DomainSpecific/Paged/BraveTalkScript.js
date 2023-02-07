// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

'use strict';

window.__firefox__.includeOnce("BraveTalkScript", function($) {
  let sendMessage = $(function() {
    return $.postNativeMessage('$<message_handler>', { 'securityToken': SECURITY_TOKEN });
  });
  
  Object.defineProperty(window, 'chrome', {
    enumerable: false,
    configurable: true,
    writable: false,
    value: {
      braveRequestAdsEnabled() {
        return sendMessage();
      }
    }
  });
  
  const launchNativeBraveTalk = $(function (url) {
    $.postNativeMessage('$<message_handler>', {
      'kind': 'launchNativeBraveTalk',
      'url': url,
      'securityToken': SECURITY_TOKEN
    });
  });
  
  if (document.location.host === "talk.brave.com") {
    const postRoom = $((event) => {
      if (event.target.tagName !== undefined && event.target.tagName.toLowerCase() == "iframe") {
        launchNativeBraveTalk(event.target.src);
        window.removeEventListener("DOMNodeInserted", postRoom)
      }
    });
    window.addEventListener("DOMNodeInserted", postRoom);
  }
});

