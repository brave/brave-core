// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUIListenerBehavior} from 'chrome://resources/js/web_ui_listener_behavior.m.js';
import {routes} from '../route.js';
import {Router} from '../router.js';
import 'chrome://resources/cr_elements/md_select_css.m.js';
import '../settings_shared_css.js';
import '../settings_vars_css.js';
import {loadTimeData} from "../i18n_setup.js"
import {BraveAppearanceBrowserProxy,  BraveAppearanceBrowserProxyImpl} from './brave_appearance_browser_proxy.js';

/**
 * 'settings-brave-appearance-theme' is the settings page area containing
 * brave's appearance related settings that located at the top of appearance
 * area.
 */
Polymer({
  is: 'settings-brave-appearance-theme',

  _template: html`{__html_template__}`,

  behaviors: [
    WebUIListenerBehavior,
  ],

  properties: {
    braveThemeList_: Array,
    braveThemeType_: Number,
  },

  /** @private {?settings.BraveAppearanceBrowserProxy} */
  browserProxy_: null,

  observers: [
    'updateSelected_(braveThemeType_, braveThemeList_)',
  ],

  /** @override */
  created: function() {
    this.browserProxy_ = BraveAppearanceBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.addWebUIListener('brave-theme-type-changed', (type) => {
      this.braveThemeType_ = type;
    })
    this.browserProxy_.getBraveThemeList().then(list => {
      this.braveThemeList_ = JSON.parse(list);
    })
    this.browserProxy_.getBraveThemeType().then(type => {
      this.braveThemeType_ = type;
    })
  },

  onBraveThemeTypeChange_: function() {
    this.browserProxy_.setBraveThemeType(Number(this.$.braveThemeType.value));
  },

  braveThemeTypeEqual_: function(theme1, theme2) {
    return theme1 === theme2;
  },

  onThemeTap_: function() {
    Router.getInstance().navigateTo(routes.THEMES);
  },

  // Wait for the dom-repeat to populate the <select> before setting
  // <select>#value so the correct option gets selected.
  updateSelected_: function() {
    this.async(() => {
      this.$.braveThemeType.value = this.braveThemeType_;
    });
  },

  useThemesSubPage_: function() {
    return loadTimeData.getString('superReferralThemeName') !== "";
  }
});
