/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

chrome.test.runTests([
  function braveWalletExtensionHasAccess() {
    if (chrome.braveWallet && chrome.braveWallet.isEnabled &&
        chrome.braveWallet.promptToEnableWallet) {
      chrome.test.succeed();
    } else {
      chrome.test.fail();
    }
  },
  function braveWalletExtensionSeedFunctionHasAccess() {
    if (chrome.braveWallet && chrome.braveWallet.getWalletSeed &&
        chrome.braveWallet.getProjectID) {
      chrome.test.succeed();
    } else {
      chrome.test.fail();
    }
  },
  function braveWalletWrongInputKeySizeFails() {
    const buf = new Uint8Array([...Array(3).keys()])
    const arraybuf = buf.buffer.slice(buf.byteOffset,
        buf.byteLength + buf.byteOffset)
    chrome.braveWallet.getWalletSeed(arraybuf, (seed) => {
      if (!seed) {
        chrome.test.succeed();
        return
      }
      chrome.test.fail();
    })
  },
  function braveWalletExtensionGetWalletSeedSameSeedOnMultipleRequests() {
    const buf = new Uint8Array([...Array(32).keys()])
    const arraybuf = buf.buffer.slice(buf.byteOffset,
        buf.byteLength + buf.byteOffset)
    chrome.braveWallet.getWalletSeed(arraybuf, (firstSeed) => {
      chrome.braveWallet.getWalletSeed(arraybuf, (secondSeed) => {
        const dataView1 = new DataView(firstSeed)
        const dataView2 = new DataView(secondSeed)
        if (dataView1.byteLength === 32 &&
            dataView1.byteLength === dataView2.byteLength) {
          var allSame = true
          for (let i = 0; i < dataView1.byteLength; i++) {
            allSame = allSame && dataView1.getUint8(i) === dataView2.getUint8(i)
          }
          if (allSame) {
            chrome.test.succeed();
            return
          }
          console.error('Seeds differ across calls!')
        }
        chrome.test.fail();
      })
    })
  },
  function braveWalletExtensionGetWalletSeedDifferentKeysDoesNotWork() {
    const buf = new Uint8Array([...Array(32).keys()])
    const arraybuf = buf.buffer.slice(buf.byteOffset,
        buf.byteLength + buf.byteOffset)
    const buf2 = new Uint8Array([11, ...Array(31).keys()])
    const arraybuf2 = buf2.buffer.slice(buf2.byteOffset, buf2.byteLength + buf2.byteOffset)
    chrome.braveWallet.getWalletSeed(arraybuf, (firstSeed) => {
      chrome.braveWallet.getWalletSeed(arraybuf2, (secondSeed) => {
        if (firstSeed && !secondSeed) {
          chrome.test.succeed();
          return
        }
        chrome.test.fail();
      })
    })
  },
  function braveWalletExtensionGetProjectIDWorks() {
    chrome.braveWallet.getProjectID((projectID) => {
      if (projectID === "test-project-id") {
        chrome.test.succeed();
        return
      }
      console.log('Failed project ID is: ' + projectID)
      chrome.test.fail();
    })
  }
]);

