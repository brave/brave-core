/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// clang-format off
// #import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';
// clang-format on

cr.define('settings', function() {
  /** @interface */
  class SocialBlockingPageProxy {
    /**
     * @param {boolean} enabled (true/false).
     */
    setSignInEnabledAtNextStartup(value) {}
    /**
     * @return {boolean}
     */
    wasSignInEnabledAtStartup() {}
  }

  /**
   * @implements {settings.SocialBlockingPageProxy}
   */
  /* #export */ class SocialBlockingPageProxyImpl {
    setSignInEnabledAtNextStartup(value) {
      chrome.settingsPrivate.setPref('signin.allowed_on_next_startup', value);
    }

    wasSignInEnabledAtStartup() {
      return loadTimeData.getBoolean('signInAllowedOnNextStartupInitialValue');
    }
  }

  cr.addSingletonGetter(SocialBlockingPageProxyImpl);

  // #cr_define_end
  return {
    SocialBlockingPageProxy: SocialBlockingPageProxy,
    SocialBlockingPageProxyImpl: SocialBlockingPageProxyImpl,
  };
});
