/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

cr.define('settings', function() {
  /** @interface */
  class BraveAppearanceBrowserProxy {
    /**
     * @return {!Promise<string>}
     */
    getBraveThemeType() {}
    /**
     * @param {string} theme name.
     */
    setBraveThemeType(theme) {}
  }

  /**
   * @implements {settings.BraveAppearanceBrowserProxy}
   */
  class BraveAppearanceBrowserProxyImpl {
    /** @override */
    getBraveThemeType() {
      return cr.sendWithPromise('getBraveThemeType');
    }

    /** @override */
    setBraveThemeType(theme) {
      chrome.send('setBraveThemeType', [theme]);
    }
  }

  cr.addSingletonGetter(BraveAppearanceBrowserProxyImpl);

  return {
    BraveAppearanceBrowserProxy: BraveAppearanceBrowserProxy,
    BraveAppearanceBrowserProxyImpl: BraveAppearanceBrowserProxyImpl,
  };
});
