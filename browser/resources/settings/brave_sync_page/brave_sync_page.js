/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * 'settings-brave-sync-page' is the settings page containing brave's
 * custom sync.
 */
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.m.js';
import 'chrome://resources/cr_elements/icons.m.js';
import '../settings_page/settings_animated_pages.m.js';
import '../settings_page/settings_subpage.m.js';
import '../md_select_css.m.js';
import '../settings_shared_css.m.js';
import '../settings_vars_css.m.js';
import './brave_sync_subpage.js';

import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js';
import {WebUIListenerBehavior} from 'chrome://resources/js/web_ui_listener_behavior.m.js';
import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {Router} from '../router.m.js';
import {SyncBrowserProxyImpl} from '../people_page/sync_browser_proxy.m.js';
import {BraveSyncBrowserProxy} from './brave_sync_browser_proxy.js';

Polymer({
  is: 'settings-brave-sync-page',

  _template: html`{__html_template__}`,

  behaviors: [
    I18nBehavior,
  ],

  properties: {},

  /** @private {?DefaultSyncBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = DefaultBraveSyncBrowserProxyImpl.getInstance();
  },

  /** @override */
  attached: function() {
  },
});