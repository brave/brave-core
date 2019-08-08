/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

chrome.test.runTests([
  function braveShieldsExtensionHasAccess() {
    if (chrome.braveShields) {
      chrome.test.succeed();
    } else {
      chrome.test.fail();
    }
  },
  function braveShieldsHasWalletAccessButNotSeed() {
    if (chrome.braveWallet && !chrome.braveWallet.getWalletSeed) {
      chrome.test.succeed();
    } else {
      chrome.test.fail();
    }
  },
]);

