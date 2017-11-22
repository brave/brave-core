/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {updateShieldsSettings} from '../api/shieldsAPI'

chrome.tabs.onActivated.addListener(() => {
  updateShieldsSettings()
    .catch(() => {
      console.error('could not update shields settings')
    })
})

chrome.tabs.onUpdated.addListener(function (tabId, changeInfo, tab) {
  updateShieldsSettings()
    .catch(() => {
      console.error('could not update shields settings')
    })
})
