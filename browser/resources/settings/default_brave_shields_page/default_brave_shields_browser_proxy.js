/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// clang-format off
// #import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';
// clang-format on

cr.define('settings', function() {
  /** @interface */
  /* #export */ class DefaultBraveShieldsBrowserProxy {
    /**
     * @return {!Promise<boolean>}
     */
    isAdControlEnabled() {}
    /**
     * @param {string} value name.
     */
    setAdControlType(value) {}

    /**
     * @return {!Promise<boolean>}
     */
    isFirstPartyCosmeticFilteringEnabled() {}
    /**
     * @param {string} value name.
     */
    setCosmeticFilteringControlType(value) {}

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
  }

  /**
   * @implements {settings.DefaultBraveShieldsBrowserProxy}
   */
  /* #export */ class DefaultBraveShieldsBrowserProxyImpl {
    /** @override */
    isAdControlEnabled() {
      return cr.sendWithPromise('isAdControlEnabled');
    }

    /** @override */
    setAdControlType(value) {
      chrome.send('setAdControlType', [value]);
    }

    /** @override */
    isFirstPartyCosmeticFilteringEnabled() {
      return cr.sendWithPromise('isFirstPartyCosmeticFilteringEnabled');
    }

    /** @override */
    setCosmeticFilteringControlType(value) {
      chrome.send('setCosmeticFilteringControlType', [value]);
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
  }

  cr.addSingletonGetter(DefaultBraveShieldsBrowserProxyImpl);

  // #cr_define_end
  return {
    DefaultBraveShieldsBrowserProxy,
    DefaultBraveShieldsBrowserProxyImpl
  };
});
