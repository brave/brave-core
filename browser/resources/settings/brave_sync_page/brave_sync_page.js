/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * 'settings-brave-sync-page' is the settings page containing brave's
 * custom sync.
 */
Polymer({
  is: 'settings-brave-sync-page',

  behaviors: [I18nBehavior, WebUIListenerBehavior],

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
    syncStatus: Object,

    /**
     * Dictionary defining page visibility.
     * @type {!PageVisibility}
     */
    pageVisibility: Object,
  },

  /** @override */
  attached: function() {
    this.addWebUIListener('sync-settings-saved', () => {
    });
  },
  /** @private */
  onSyncTap_: function() {
    // Users can go to sync subpage regardless of sync status.
    settings.navigateTo(settings.routes.BRAVE_SYNC_SETUP);
  },

});
