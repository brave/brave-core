/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * 'settings-brave-sync-page' is the settings page containing brave's
 * custom sync.
 */
Polymer({
  is: 'settings-brave-sync-page',

  behaviors: [
    I18nBehavior,
    WebUIListenerBehavior,
  ],

  properties: {
    /**
     * Preferences state.
     */
    prefs: {
      type: Object,
      notify: true,
    },

    /**
     * The current sync status, supplied by SyncBrowserProxy.
     * @type {?settings.SyncStatus}
     */

    /**
     * Dictionary defining page visibility.
     * @type {!PageVisibility}
     */
    pageVisibility: Object,
    syncStatus_: Object,
  },

  /** @private {?settings.SyncBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.SyncBrowserProxyImpl.getInstance();
  },

  /** @private */
  getSyncLabel_(syncStatus) {
    const isAlreadySetup = this.syncStatus !== undefined &&
        !this.syncStatus.firstSetupInProgress;
    const key = isAlreadySetup ? 'braveSyncManageActionLabel' : 'braveSyncSetupActionLabel';
    return I18nBehavior.i18n(key);
  },

  /** @override */
  attached: function() {
    const onSyncStatus = this.handleSyncStatus_.bind(this)
    this.browserProxy_.getSyncStatus().then(onSyncStatus);
    this.addWebUIListener('sync-status-changed',onSyncStatus);
    this.addWebUIListener(
        'sync-prefs-changed', this.handleSyncPrefsChanged_.bind(this));
  },

  /** @private */
  onSyncTap_: function() {
    // Users can go to sync subpage regardless of sync status.
    const router = settings.Router.getInstance();
    router.navigateTo(router.getRoutes().BRAVE_SYNC_SETUP);
  },

  /**
   * Handler for when the sync state is pushed from the browser.
   * @param {?settings.SyncStatus} syncStatus
   * @private
   */
  handleSyncStatus_: function(syncStatus) {
    this.syncStatus_ = syncStatus;
  },

  /**
   * Handler for when the sync preferences are updated.
   * @private
   */
  handleSyncPrefsChanged_: async function(syncPrefs) {
    // Enforce encryption
    if (this.syncStatus_ && !this.syncStatus_.firstSetupInProgress) {
      if (!syncPrefs.encryptAllData) {
        const syncCode = await this.browserProxy_.getSyncCode()
        syncPrefs.encryptAllData = true;
        syncPrefs.setNewPassphrase = true;
        syncPrefs.passphrase = syncCode;
        console.debug('sync set encryption', syncPrefs)
        await this.browserProxy_.setSyncEncryption(syncPrefs)
      } else if (syncPrefs.passphraseRequired) {
        const syncCode = await this.browserProxy_.getSyncCode()
        syncPrefs.setNewPassphrase = false;
        syncPrefs.passphrase = syncCode;
        console.debug('sync set encryption', syncPrefs)
        await this.browserProxy_.setSyncEncryption(syncPrefs)
      }
    }
  },
});
