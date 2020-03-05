/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

require('./events/windowsEvents')
require('./events/tabsEvents')
require('./events/shieldsEvents')
require('./events/runtimeEvents')
require('./events/webNavigationEvents')
require('./events/cosmeticFilterEvents')
require('./events/settingsEvents')
// Only do detection events if the wallet API is available
if (chrome.braveWallet) {
  require('./events/dappDetectionEvents')
}
