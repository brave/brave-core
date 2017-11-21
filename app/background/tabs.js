/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {updateShieldsSettings} from './api/shields'

chrome.tabs.onActivated.addListener(() => {
  updateShieldsSettings()
})

chrome.tabs.onUpdated.addListener(function (tabId, changeInfo, tab) {
  updateShieldsSettings()
})
