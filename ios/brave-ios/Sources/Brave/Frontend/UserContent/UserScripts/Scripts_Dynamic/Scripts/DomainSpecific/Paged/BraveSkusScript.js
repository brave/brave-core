// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict';

window.__firefox__.includeOnce("BraveSkusScript", function($) {
  let sendMessage = $(function(method_id, data) {
    return $.postNativeMessage('$<message_handler>', {
      'securityToken': SECURITY_TOKEN,
      'method_id': method_id,
      'data': data
    });
  });

  if (!window.chrome) {
    window.chrome = {};
  }
  
  // Request VPN Receipt
  sendMessage('$<setLocalStorageReceipt>', { "message": "vpn" })
  .then(function(jsonData) {
    if (jsonData) {
      window.localStorage.setItem(jsonData["key"], jsonData["data"]);
    } else {
      console.error("No VPN Subscription Data");
    }
  }).catch(function(err) {
    console.error(`An error occurred setting the local storage receipt: ${err}`);
  });
  
  // Request Leo Receipt
  sendMessage('$<setLocalStorageReceipt>', { "message": "leo" })
  .then(function(jsonData) {
    if (jsonData) {
      window.localStorage.setItem("braveLeo.orderId", jsonData["braveLeo.orderId"])
      window.localStorage.setItem(jsonData["key"], jsonData["data"]);
    } else {
      console.error("No Leo Subscription Data");
    }
  }).catch(function(err) {
    console.error(`An error occurred setting the local storage receipt: ${err}`);
  });

  Object.defineProperty(window.chrome, 'braveSkus', {
    enumerable: false,
    configurable: false,
    writable: false,
      value: {
        refresh_order(orderId) {
          return sendMessage('$<refreshOrder>', { orderId });
        },

        fetch_order_credentials(orderId) {
          return sendMessage('$<fetchOrderCredentials>', { orderId });
        },

        prepare_credentials_presentation(domain, path) {
          return sendMessage('$<prepareCredentialsPresentation>', { domain, path });
        },

        credential_summary(domain) {
          return sendMessage('$<credentialsSummary>', { domain });
        }
      }
  });
});
