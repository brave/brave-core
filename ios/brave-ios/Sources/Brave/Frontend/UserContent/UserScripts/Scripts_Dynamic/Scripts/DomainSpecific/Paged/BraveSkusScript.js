// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

'use strict';

window.__firefox__.includeOnce("BraveSkusScript", function($) {
  let sendMessage = $(function(method_id, data) {
    return $.postNativeMessage('$<message_handler>', {'securityToken': SECURITY_TOKEN,'method_id': method_id, data: data});
  });

  if (!window.chrome) {
    window.chrome = {};
  }

  Object.defineProperty(window.chrome, 'braveSkus', {
    enumerable: false,
    configurable: false,
    writable: false,
      value: {
        refresh_order(orderId) {
          return sendMessage(1, { orderId });
        },
          
        fetch_order_credentials(orderId) {
          return sendMessage(2, { orderId });
        },
          
        prepare_credentials_presentation(domain, path) {
          return sendMessage(3, { domain, path });
        },
          
        credential_summary(domain) {
          return sendMessage(4, { domain });
        }
      }
  });
});
