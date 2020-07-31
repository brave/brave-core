/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * 'settings-social-blocking-page' is the settings page containing brave's
 * social blocking options
 */
Polymer({
  is: 'settings-social-blocking-page',

  properties: {},

  /** @private {?settings.SocialBlockingPageProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.SocialBlockingPageProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.shouldShowRestart_ = this.shouldShowRestart_.bind(this)
    this.restartBrowser_ = this.restartBrowser_.bind(this)
  },

  shouldShowRestart_: function(value) {
    return this.browserProxy_.wasSignInEnabledAtStartup() != value;
  },

  setSignInEnabledAtNextStartup_: function() {
    this.browserProxy_.setSignInEnabledAtNextStartup(this.$.googleLoginControlType.checked);
  },

  restartBrowser_: function(e) {
    e.stopPropagation();
    window.open("chrome://restart", "_self");
  },
});
