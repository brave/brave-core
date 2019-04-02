// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

(function() {
  Polymer({
    is: 'settings-brave-reset-rewards',
    behaviors: [settings.RouteObserverBehavior],

  /**
   * settings.RouteObserverBehavior
   * @param {!settings.Route} route
   * @protected
   */
  currentRouteChanged: function(route) {
    const lazyRender = (this.$.resetRewardsDialog);

    if (route == settings.routes.RESET_REWARDS_DIALOG) {
      (lazyRender.get())
          .show();
    }
  },

  /** @private */
  onShowResetRewardsDialog_: function() {
    settings.navigateTo(
        settings.routes.RESET_REWARDS_DIALOG);
  },

  /** @private */
  onResetRewardsDialogClose_: function() {
    settings.navigateToPreviousRoute();
    cr.ui.focusWithoutInk(assert(this.$.resetRewards));
  },

  });
  })();
