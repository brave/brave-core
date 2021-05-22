/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

function getConfig(expectedSuccess, expectedConfig) {
  chrome.ipfs.getConfig((success, config) => {
    if (success === expectedSuccess && config === expectedConfig) {
      chrome.test.succeed();
    } else {
      chrome.test.fail();
    }
  })
}

function resolveMethodChangeIsReflected(expectedType) {
  chrome.ipfs.getResolveMethodType((type) => {
    if (type === expectedType) {
      chrome.test.succeed();
    } else {
      chrome.test.fail();
    }
  })
}

function executableAvailableChangeIsReflected(expectedAvail) {
  chrome.ipfs.getExecutableAvailable((avail) => {
    if (avail === expectedAvail) {
      chrome.test.succeed();
    } else {
      chrome.test.fail();
    }
  })
}

function launchSuccess() {
  chrome.ipfs.launch((success) => {
    if (success) {
      chrome.test.succeed();
    } else {
      chrome.test.fail();
    }
  })
}

function launchFail() {
  chrome.ipfs.launch((success) => {
    if (success) {
      chrome.test.fail();
    } else {
      chrome.test.succeed();
    }
  })
}

function shutdownSuccess() {
  chrome.ipfs.shutdown((success) => {
    if (success) {
      chrome.test.succeed();
    } else {
      chrome.test.fail();
    }
  })
}

function resolveIPFSURIMatches(uri, expected_url) {
  chrome.ipfs.resolveIPFSURI(uri, (gateway_url) => {
    if (gateway_url === expected_url) {
      chrome.test.succeed();
    } else {
      chrome.test.fail();
    }
  })
}

function testBasics() {
  chrome.test.runTests([
    function ipfsCompanionExtensionHasAccess() {
      if (chrome.ipfs &&
          chrome.ipfs.getIPFSEnabled &&
          chrome.ipfs.getResolveMethodType &&
          chrome.ipfs.launch &&
          chrome.ipfs.shutdown &&
          chrome.ipfs.getConfig &&
          chrome.ipfs.getExecutableAvailable &&
          chrome.ipfs.resolveIPFSURI &&
          chrome.ipfs.getIpfsPeersList &&
          chrome.ipfs.addIpfsPeer &&
          chrome.ipfs.removeIpfsPeer &&
          chrome.ipfs.getIpnsKeysList &&
          chrome.ipfs.addIpnsKey &&
          chrome.ipfs.removeIpnsKey) {
        chrome.test.succeed();
      } else {
        chrome.test.fail();
      }
    },
    function getIPFSEnabled() {
      chrome.ipfs.getIPFSEnabled((enabled) => {
        if (enabled) {
          chrome.test.succeed();
        } else {
          chrome.test.fail();
        }
      })
    },
    function getResolveMethodType() {
      chrome.ipfs.getResolveMethodType((type) => {
        if (type === 'ask') {
          chrome.test.succeed();
        } else {
          chrome.test.fail();
        }
      })
    },
  ]);
}
