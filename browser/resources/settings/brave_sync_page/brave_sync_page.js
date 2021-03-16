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
import '../settings_page/settings_animated_pages.js';
import '../settings_page/settings_subpage.js';
import '../settings_shared_css.m.js';
import '../settings_vars_css.m.js';
import './brave_sync_subpage.js';

import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js';
import {WebUIListenerBehavior} from 'chrome://resources/js/web_ui_listener_behavior.m.js';
import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {Router} from '../router.js';
import {SyncBrowserProxyImpl} from '../people_page/sync_browser_proxy.m.js';
import {BraveSyncBrowserProxy} from './brave_sync_browser_proxy.js';

Polymer({
  is: 'settings-brave-sync-page',

  _template: html`{__html_template__}`,

  behaviors: [
    I18nBehavior,
    WebUIListenerBehavior,
  ],

  properties: {
    /**
     * The current sync status, supplied by SyncBrowserProxy.
     * @type {?SyncStatus}
     */
    syncStatus_: Object,
    isEncryptionSet_: {
      type: Boolean,
      value: false
    },
    syncLabel_: {
      type: String,
      computed: 'computeSyncLabel_(syncStatus_.firstSetupInProgress)'
    },
  },

  /** @private {?SyncBrowserProxy} */
  browserProxy_: null,
  /** @private */
  braveBrowserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = SyncBrowserProxyImpl.getInstance();
    this.braveBrowserProxy_ = BraveSyncBrowserProxy.getInstance();
  },

  /** @private */
  computeSyncLabel_() {
    const isAlreadySetup = this.syncStatus_ !== undefined &&
        !this.syncStatus_.firstSetupInProgress;
    const key = isAlreadySetup ? 'braveSyncManageActionLabel' : 'braveSyncSetupActionLabel';
    return I18nBehavior.i18n(key);
  },

  /** @override */
  attached: function() {
    const onSyncStatus = this.handleSyncStatus_.bind(this)
    this.browserProxy_.getSyncStatus().then(onSyncStatus);
    this.addWebUIListener(
      'sync-prefs-changed', this.handleSyncPrefsChanged_.bind(this));
    this.addWebUIListener('sync-status-changed', onSyncStatus);
  },

  /** @private */
  onSyncTap_: function() {
    // Users can go to sync subpage regardless of sync status.
    const router = Router.getInstance();
    router.navigateTo(router.getRoutes().BRAVE_SYNC_SETUP);
  },

  /**
   * Handler for when the sync state is pushed from the browser.
   * @param {?SyncStatus} syncStatus
   * @private
   */
  handleSyncStatus_: async function(syncStatus) {
    this.syncStatus_ = syncStatus;
  },

  /**
   * Handler for when the sync preferences are updated.
   * @private
   */
  handleSyncPrefsChanged_: async function(syncPrefs) {
    if (this.syncStatus_ && !this.syncStatus_.firstSetupInProgress) {
      const syncCode = await this.braveBrowserProxy_.getSyncCode()
      if (syncPrefs.passphraseRequired) {
        await this.browserProxy_.setDecryptionPassphrase(syncCode);
      } else if (!this.isEncryptionSet_) {
        this.browserProxy_.setEncryptionPassphrase(syncCode)
        .then(successfullySet => {
          this.isEncryptionSet_ = successfullySet
        })
      }
    }
  },
});
