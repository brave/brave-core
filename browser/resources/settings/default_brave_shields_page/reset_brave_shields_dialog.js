/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * @fileoverview
 *
 * 'settings-reset-brave-shields-dialog' is the dialog shown for clearing shields
 * settings.
 */
Polymer({
  is: 'settings-reset-brave-shields-dialog',

  properties: {
    /** @private */
    clearingInProgress_: {
      type: Boolean,
      value: false,
    },
  },

  /** @private {?settings.DefaultBraveShieldsBrowserProxy} */
  browserProxy_: null,

  /** @override */
  ready: function() {
    this.browserProxy_ = settings.DefaultBraveShieldsBrowserProxyImpl.getInstance();
  },

  show: function() {
    if (!this.$.dialog.open) {
      this.$.dialog.showModal();
    }
  },

  /** @private */
  onCancelTap_: function() {
    this.cancel();
  },

  cancel: function() {
    if (this.$.dialog.open) {
      this.$.dialog.cancel();
    }
  },

  /** @private */
  onResetTap_: function() {
    this.clearingInProgress_ = true;
    this.browserProxy_
        .performResetShieldsSettings()
        .then(() => {
          this.clearingInProgress_ = false;
          if (this.$.dialog.open) {
            this.$.dialog.close();
          }
        });
  },
});
