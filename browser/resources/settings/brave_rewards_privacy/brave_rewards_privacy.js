// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

(function() {
Polymer({
  is: 'settings-brave-rewards-privacy',

  behaviors: [
    settings.RouteObserverBehavior,
    I18nBehavior,
    WebUIListenerBehavior,
  ],

  properties: {
    /** @private */
    showClearRewardsDataDialog_: Boolean,
  },

  /** @protected */
  currentRouteChanged: function() {
    this.showClearRewardsDataDialog_ =
        settings.getCurrentRoute() == settings.routes.CLEAR_REWARDS_DATA;
  },

  /** @private */
  onClearRewardsDataTap_: function() {
    settings.navigateTo(settings.routes.CLEAR_REWARDS_DATA);
  },

  /** @private */
  onDialogClosed_: function() {
    settings.navigateTo(settings.routes.CLEAR_REWARDS_DATA.parent);
    cr.ui.focusWithoutInk(assert(this.$.clearRewardsDataTrigger));
  },
});
})();
