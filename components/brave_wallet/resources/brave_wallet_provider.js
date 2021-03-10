// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

(function() {
    if (window.ethereum) {
      return
    }

    window.ethereum = {
      request: function(obj) {
        return navigator.brave_wallet.request(obj).then(
                function(value) { console.log('window.ethereum.request resolved: ' + value) },
                function(error) { console.log('window.ethereum.request rejected: ' + error) }
            );
      }
    }
})()
