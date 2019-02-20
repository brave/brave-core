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
  allow: 'Allow',
  // Footer
  cancel: 'Cancel',
  goBack: 'Go back',
  applyOnce: 'Apply once',
  changeDefaults: 'Change global shield defaults'

  // totalBlocked: 'Total blocked',
  // disabledMessage: 'You’re browsing this site without any privacy and security protections.',
  // blockAds: 'Ads and trackers blocked',
  // blockPopups: 'Pop-ups blocked',
  // blockImages: 'Images blocked',
  // block3partyCookies: '3rd-party cookies blocked',
  // allowAllCookies: 'All cookies allowed',
  // blockAllCookies: 'Cookies blocked',
  // blockSomeScripts: 'Some scripts blocked',
  // blockAllScripts: 'Scripts blocked',
  // allowAllScripts: 'All scripts allowed',
  // allowAllFingerprinting: 'All device recognition allowed',
  // blockAllFingerprinting: 'Device recognition blocked',
  // block3partyFingerprinting: '3rd-party device recognition blocked',
  // blockPishing: 'Phishing/malware attempts blocked',
  // connectionsEncrypted: 'Connections encrypted',
  // editDefaults: 'Global shield defaults',
  // scriptsOnThisSite: 'Scripts on this site',
  // blockedScripts: 'Blocked scripts',
  // allowedScripts: 'Allowed scripts',
  // allowAll: 'Allow all',
  // allow: 'Allow',
  // allowed: 'Allowed',
  // blockAll: 'Block all',
  // block: 'Block',
  // blocked: 'Blocked',
  // apply: 'Apply',
  // applyOnce: 'Apply once',
  // applyUntilRestart: 'Apply until restart',
  // alwaysApply: 'Always apply',
  // undo: 'Undo',

  // cookiesOnThisSite: 'Cookies on this site',
  // deviceRecognitionAttempts: 'Device recognition attempts'
}

export default locale

export const getLocale = (word: string) => locale[word]
