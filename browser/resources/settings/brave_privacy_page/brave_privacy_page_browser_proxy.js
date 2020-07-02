/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// clang-format off
// #import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';
// clang-format on

cr.define('settings', function() {
  /** @interface */
  /* #export */ class BravePrivacyBrowserProxy {
    /**
     * @return {!Promise<string>}
     */
    getWebRTCPolicy() {}
    /**
     * @param {string} policy name.
     */
    setWebRTCPolicy(policy) {}
    /**
     * @return {!Promise<string>}
     */
    getP3AEnabled() {}
    /**
     * @param {boolean} enabled (true/false).
     */
    setP3AEnabled(value) {}
    /**
     * @return {boolean}
     */
    wasPushMessagingEnabledAtStartup() {}
  }

  /**
   * @implements {settings.BravePrivacyBrowserProxy}
   */
  /* #export */ class BravePrivacyBrowserProxyImpl {
    /** @overrides */
    getWebRTCPolicy() {
      return cr.sendWithPromise('getWebRTCPolicy');
    }

    setWebRTCPolicy(policy) {
      chrome.send('setWebRTCPolicy', [policy]);
    }

    getP3AEnabled() {
      return cr.sendWithPromise('getP3AEnabled');
    }

    setP3AEnabled(value) {
      chrome.send('setP3AEnabled', [value])
    }

    wasPushMessagingEnabledAtStartup() {
      return loadTimeData.getBoolean('pushMessagingEnabledAtStartup');
    }
  }

  cr.addSingletonGetter(BravePrivacyBrowserProxyImpl);

  // #cr_define_end
  return {
    BravePrivacyBrowserProxy: BravePrivacyBrowserProxy,
    BravePrivacyBrowserProxyImpl: BravePrivacyBrowserProxyImpl,
  };
});
