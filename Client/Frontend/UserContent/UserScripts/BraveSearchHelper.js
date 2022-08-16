// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

'use strict';

window.__firefox__.includeOnce("BraveSearchHelper", function() {
  function sendMessage(method_id) {
    return webkit.messageHandlers.$<brave-search-helper>.postMessage({ 'securitytoken': '$<security_token>' ,'method_id': method_id});
  }
  
  Object.defineProperty(window, 'brave', {
    enumerable: false,
    configurable: true,
    writable: false,
    value: {
      getCanSetDefaultSearchProvider() {
        return sendMessage(1);
      },
      setIsDefaultSearchProvider() {
        return sendMessage(2);
      }
    }
  });
});
