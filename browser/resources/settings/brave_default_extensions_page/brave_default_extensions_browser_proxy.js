/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

cr.define('settings', function() {
  /** @interface */
  class BraveDefaultExtensionsBrowserProxy {
    /**
     * @param {boolean} value name.
     */
    setWebTorrentEnabled(value) {}
    setBraveWalletEnabled(value) {}
    setHangoutsEnabled(value) {}
    setIPFSCompanionEnabled(value) {}
    setTorEnabled(value) {}
    getTorEnabled() {}
    getEnableTorOption() {}
    getRestartNeeded() {}
  }

  /**
   * @implements {settings.BraveDefaultExtensionsBrowserProxy}
   */
  class BraveDefaultExtensionsBrowserProxyImpl {
    /** @override */
    setWebTorrentEnabled(value) {
      chrome.send('setWebTorrentEnabled', [value]);
    }
    setBraveWalletEnabled(value) {
      chrome.send('setBraveWalletEnabled', [value]);
    }
    setHangoutsEnabled(value) {
      chrome.send('setHangoutsEnabled', [value]);
    }
    setIPFSCompanionEnabled(value) {
      chrome.send('setIPFSCompanionEnabled', [value]);
    }
    setMediaRouterEnabled(value) {
      chrome.send('setMediaRouterEnabled', [value]);
    }
    setTorEnabled(value) {
      chrome.send('setTorEnabled', [value]);
    }
    getTorEnabled() {
      return cr.sendWithPromise('getTorEnabled');
    }
    getEnableTorOption() {
      return cr.sendWithPromise('getEnableTorOption');
    }
    getRestartNeeded() {
      return cr.sendWithPromise('getRestartNeeded');
    }
  }

  cr.addSingletonGetter(BraveDefaultExtensionsBrowserProxyImpl);

  return {
    BraveDefaultExtensionsBrowserProxy,
    BraveDefaultExtensionsBrowserProxyImpl
  };
});
