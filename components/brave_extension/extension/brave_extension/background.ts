/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import './background/greaselion'
import './background/webDiscoveryProject'
require('./background/events')

if (chrome.test) {
  chrome.test.sendMessage('brave-extension-enabled')
}
