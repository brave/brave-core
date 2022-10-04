// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {WebUIListenerBehavior} from 'chrome://resources/cr_elements/web_ui_listener_behavior.js';

(function() {
  'use strict';

  /**
   * 'settings-brave-new-tab-page' is the settings page containing
   * brave's new tab features.
   */
  Polymer({
    is: 'settings-brave-new-tab-page',

    /** @private {?settings.BraveNewTabBrowserProxy} */
    browserProxy_: null,

    behaviors: [
      WebUIListenerBehavior,
    ],

    properties: {
      newTabShowOptions_: Array,
      shouldNewTabShowDashboardSettings_: Boolean,
    },

    /** @override */
    created: function() {
      this.browserProxy_ = settings.BraveNewTabBrowserProxyImpl.getInstance();
      this.showNewTabDashboardSettings_ = false;
    },

    /** @override */
    ready: function() {
      this.openNewTabPage_ = this.openNewTabPage_.bind(this)

      this.browserProxy_.getNewTabShowsOptionsList().then(list => {
        this.newTabShowOptions_ = list;
      })
      this.browserProxy_.shouldShowNewTabDashboardSettings().then(showNewTabDashboardSettings => {
        this.showNewTabDashboardSettings_ = showNewTabDashboardSettings;
      })

      this.addWebUIListener('show-new-tab-dashboard-settings-changed', (show) => {
        this.showNewTabDashboardSettings_ = show
      })
    },

    openNewTabPage_: function () {
      window.open("chrome://newTab?openSettings=1", "_self");
    }
  });
})();
