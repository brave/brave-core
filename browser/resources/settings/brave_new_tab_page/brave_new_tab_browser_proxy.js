/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// clang-format off
// #import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';
// clang-format on

cr.define('settings', function() {
  /** @interface */
  /* #export */ class BraveNewTabBrowserProxy {
    /**
     * @return {!Promise<Array>}
     */
    getNewTabShowsOptionsList() {}

    /**
     * @return {!Promise<Boolean>}
     */
    shouldShowNewTabDashboardSettings() {}
  }

  /**
   * @implements {settings.BraveNewTabBrowserProxy}
   */
  /* #export */ class BraveNewTabBrowserProxyImpl {
    /** @override */
    getNewTabShowsOptionsList() {
      return cr.sendWithPromise('getNewTabShowsOptionsList')
    }

    /** @override */
    shouldShowNewTabDashboardSettings() {
      return cr.sendWithPromise('shouldShowNewTabDashboardSettings')
    }
  }

  cr.addSingletonGetter(BraveNewTabBrowserProxyImpl);

  // #cr_define_end
  return {
    BraveNewTabBrowserProxy,
    BraveNewTabBrowserProxyImpl
  };
});
