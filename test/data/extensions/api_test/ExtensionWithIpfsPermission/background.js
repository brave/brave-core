/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 function testBasics() {
  chrome.test.runTests([
    function ipfsExtensionHasAccess() {
      if (chrome.ipfs &&
          chrome.ipfs.getIPFSEnabled &&
          chrome.ipfs.getResolveMethodType &&
          chrome.ipfs.resolveIPFSURI &&
          !chrome.ipfs.getIpfsPeersList &&
          !chrome.ipfs.addIpfsPeer &&
          !chrome.ipfs.removeIpfsPeer &&
          !chrome.ipfs.getIpnsKeysList &&
          !chrome.ipfs.addIpnsKey &&
          !chrome.ipfs.rotateKey &&
          !chrome.ipfs.removeIpnsKey &&
          !chrome.ipfs.validateGatewayUrl) {
        chrome.test.succeed();
      } else {
        chrome.test.fail();
      }
    },
  ]);
}
