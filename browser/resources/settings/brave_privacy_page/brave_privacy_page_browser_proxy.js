/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

cr.define('settings', function() {
  /** @interface */
  class BravePrivacyBrowserProxy {
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
  class BravePrivacyBrowserProxyImpl {
    /** @override */
    getWebRTCPolicy() {
      return cr.sendWithPromise('getWebRTCPolicy');
    }

    /** @override */
    setWebRTCPolicy(policy) {
      chrome.send('setWebRTCPolicy', [policy]);
    }

    /** @override */
    getP3AEnabled() {
      return cr.sendWithPromise('getP3AEnabled');
    }

    /** @override */
    setP3AEnabled(value) {
      chrome.send('setP3AEnabled', [value])
    }

    wasPushMessagingEnabledAtStartup() {
      return loadTimeData.getBoolean('pushMessagingEnabledAtStartup');
    }
  }

  cr.addSingletonGetter(BravePrivacyBrowserProxyImpl);

  return {
    BravePrivacyBrowserProxy: BravePrivacyBrowserProxy,
    BravePrivacyBrowserProxyImpl: BravePrivacyBrowserProxyImpl,
  };
});
