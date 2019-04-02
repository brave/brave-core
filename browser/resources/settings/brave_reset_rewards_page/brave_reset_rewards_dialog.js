// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

Polymer({
  is: 'settings-brave-reset-rewards-dialog',

  properties: {

    /** @private */
    clearingInProgress_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showFooter_: {
      type: Boolean,
      value: false,
    }
  },

  /** @private {?settings.ResetRewardsProxy} */
  browserProxy_: null,

  /** @override */
  ready: function() {
    this.browserProxy_ = settings.ResetRewardsProxyImpl.getInstance();
  },

  show: function() {
    this.$.dialog.showModal();
  },

  /** @private */
  onCancelTap_: function() {
    if (this.$.dialog.open) {
      this.$.dialog.cancel();
    }
  },

  /** @private */
  onResetTap_: function() {
    this.clearingInProgress_ = true;
    this.browserProxy_
        .performRewardsReset()
        .then((success) => {
          this.clearingInProgress_ = false;
          if (this.$.dialog.open && success) {
            this.$.dialog.close();
          } else {
            this.showFooter_ = true
          }
          this.fire('reset-done');
        });
  },
});
