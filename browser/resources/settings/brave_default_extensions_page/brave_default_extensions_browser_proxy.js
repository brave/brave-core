/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// clang-format off
// #import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';
// clang-format on

cr.define('settings', function () {
  /** @interface */
  /* #export */ class BraveDefaultExtensionsBrowserProxy {
    /**
     * @param {boolean} value name.
     */
    setWebTorrentEnabled (value) {}
    setBraveWalletEnabled (value) {}
    setHangoutsEnabled (value) {}
    setIPFSCompanionEnabled (value) {}
    setTorEnabled (value) {}
    isTorEnabled () {}
    isTorManaged () {}
    setWidevineEnabled() {}
    isWidevineEnabled() {}
    getRestartNeeded () {}
    getWeb3ProviderList () {}
    wasSignInEnabledAtStartup () {}
    getIPFSResolveMethodList () {}
    getIPFSEnabled () {}
  }

  /**
   * @implements {settings.BraveDefaultExtensionsBrowserProxy}
   */
  /* #export */ class BraveDefaultExtensionsBrowserProxyImpl {
    /** @override */
    setWebTorrentEnabled (value) {
      chrome.send('setWebTorrentEnabled', [value])
    }

    setBraveWalletEnabled (value) {
      chrome.send('setBraveWalletEnabled', [value])
    }

    setHangoutsEnabled (value) {
      chrome.send('setHangoutsEnabled', [value])
    }

    setIPFSCompanionEnabled (value) {
      chrome.send('setIPFSCompanionEnabled', [value])
    }

    setMediaRouterEnabled (value) {
      chrome.send('setMediaRouterEnabled', [value])
    }

    setTorEnabled (value) {
      chrome.send('setTorEnabled', [value])
    }

    isTorEnabled () {
      return cr.sendWithPromise('isTorEnabled')
    }

    isTorManaged () {
      return cr.sendWithPromise('isTorManaged')
    }

    setWidevineEnabled (value) {
      chrome.send('setWidevineEnabled', [value])
    }

    isWidevineEnabled () {
      return cr.sendWithPromise('isWidevineEnabled')
    }

    getRestartNeeded () {
      return cr.sendWithPromise('getRestartNeeded')
    }

    /** @override */
    getWeb3ProviderList () {
      return new Promise(resolve => chrome.braveWallet.getWeb3ProviderList(resolve))
    }

    wasSignInEnabledAtStartup () {
      return loadTimeData.getBoolean('signInAllowedOnNextStartupInitialValue')
    }

    /** @override */
    getIPFSResolveMethodList () {
      return new Promise(resolve => {
        if (!chrome.ipfs) {
          resolve(false)
          return
        }
        chrome.ipfs.getResolveMethodList(resolve)
      })
    }

    /** @override */
    getIPFSEnabled () {
      return new Promise(resolve => {
        if (!chrome.ipfs) {
          resolve(false)
          return
        }
        chrome.ipfs.getIPFSEnabled(resolve)
      })
    }
  }

  cr.addSingletonGetter(BraveDefaultExtensionsBrowserProxyImpl)

  // #cr_define_end
  return {
    BraveDefaultExtensionsBrowserProxy,
    BraveDefaultExtensionsBrowserProxyImpl
  }
})
