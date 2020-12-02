/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

function testBasics() {
  chrome.test.runTests([
    function braveShieldsHasWalletAccessButNotSeed() {
      if (chrome.braveWallet && !chrome.braveWallet.getWalletSeed &&
          !chrome.braveWallet.getBitGoSeed &&
          !chrome.braveWallet.getProjectID &&
          !chrome.braveWallet.getBraveKey) {
        chrome.test.succeed();
      } else {
        chrome.test.fail();
      }
    },
  ]);
}
