/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// content script listener for dapp detection.
chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  switch (msg.type) {
    case 'dappAvailable': {
      chrome.tabs.query({ active: true, currentWindow: true }, function (tabs: chrome.tabs.Tab[]) {
        // Only notify when this message is comes from active tab.
        if (msg.location === tabs[0].url) {
          chrome.braveShields.dappAvailable(tabs[0].id)
        }
      })
      break
    }
  }
})
