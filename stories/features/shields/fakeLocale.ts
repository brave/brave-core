/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const locale: { [key: string]: string } = {
  // Header
  shields: 'Shields',
  up: 'up',
  down: 'down',
  forThisSite: 'for this site',
  enabledMessage: 'If a site appears broken, try shields down',
  disabledMessage: 'You’re browsing this site without any privacy and security protections.',
  // Total stats blocked
  itemsBlocked: 'Items blocked',
  itemBlocked: 'Item blocked',
  and: 'and',
  connectionsUpgraded: 'connections upgraded',
  connectionUpgraded: 'connection upgraded',
  // Controls
  thirdPartyTrackersBlocked: '3rd-party trackers blocked',
  connectionsUpgradedHTTPSCapital: 'Connections upgraded to HTTPS',
  connectionUpgradedHTTPSCapital: 'Connection upgraded to HTTPS',
  scriptsBlocked: 'Scripts blocked',
  allowScriptsOnce: 'Allow scripts once',
  thirdPartyCookiesBlocked: '3rd-party cookies blocked',
  allCookiesBlocked: 'Cookies blocked',
  allCookiesAllowed: 'All cookies allowed',
  thirdPartyFingerprintingBlocked: '3rd-party device recognition blocked',
  allFingerprintingBlocked: 'Device recognition blocked',
  allFingerprintingAllowed: 'All device recognition allowed',
  // static list
  deviceRecognitionAttempts: 'Device recognition attempts',
  scriptsOnThisSite: 'Scripts on this site',
  // scripts list
  blockedScripts: 'Blocked scripts',
  allowedScripts: 'Allowed scripts',
  blockAll: 'Block all',
  allowAll: 'Allow all',
  block: 'Block',
  blockedOnce: 'Blocked once',
  allow: 'Allow',
  allowedOnce: 'Allowed once',
  // Footer
  cancel: 'Cancel',
  goBack: 'Go back',
  applyOnce: 'Apply once',
  changeDefaults: 'Change global shield defaults'
}

export default locale

export const getLocale = (word: string) => locale[word]
