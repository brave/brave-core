/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

cr.define('settings', function() {
  /** @interface */
  class DefaultBraveShieldsBrowserProxy {
    /**
     * @return {!Promise<string>}
     */
    getAdControlType() {}
    /**
     * @param {string} value name.
     */
    setAdControlType(value) {}

    /**
     * @return {!Promise<string>}
     */
    getCookieControlType() {}
    /**
     * @param {string} value name.
     */
    setCookieControlType(value) {}

    /**
     * @return {!Promise<string>}
     */
    getFingerprintingControlType() {}
    /**
     * @param {string} value name.
     */
    setFingerprintingControlType(value) {}

    /**
     * @param {string} value name.
     */
    setHTTPSEverywhereEnabled(value) {}

    /**
     * @param {string} value name.
     */
    setNoScriptControlType(value) {}

    /**
     * @return {!Promise<>}
     */
    performResetShieldsSettings() {}
  }

  /**
   * @implements {settings.DefaultBraveShieldsBrowserProxy}
   */
  class DefaultBraveShieldsBrowserProxyImpl {
    /** @override */
    getAdControlType() {
      return cr.sendWithPromise('getAdControlType');
    }

    /** @override */
    setAdControlType(value) {
      chrome.send('setAdControlType', [value]);
    }

    /** @override */
    getCookieControlType() {
      return cr.sendWithPromise('getCookieControlType');
    }

    /** @override */
    setCookieControlType(value) {
      chrome.send('setCookieControlType', [value]);
    }

    /** @override */
    getFingerprintingControlType() {
      return cr.sendWithPromise('getFingerprintingControlType');
    }

    /** @override */
    setFingerprintingControlType(value) {
      chrome.send('setFingerprintingControlType', [value]);
    }

    /** @override */
    setHTTPSEverywhereEnabled(value) {
      chrome.send('setHTTPSEverywhereEnabled', [value]);
    }

    /** @override */
    setNoScriptControlType(value) {
      chrome.send('setNoScriptControlType', [value]);
    }

    /** @override */
    performResetShieldsSettings() {
      return cr.sendWithPromise('resetShieldsSettings');
    }
  }

  cr.addSingletonGetter(DefaultBraveShieldsBrowserProxyImpl);

  return {
    DefaultBraveShieldsBrowserProxy,
    DefaultBraveShieldsBrowserProxyImpl
  };
});
