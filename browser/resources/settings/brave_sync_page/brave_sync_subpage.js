/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {

/**
 * @fileoverview
 * 'settings-sync-page' is the settings page containing sync settings.
 */
Polymer({
  is: 'settings-brave-sync-subpage',

  behaviors: [
    WebUIListenerBehavior,
    settings.RouteObserverBehavior,
  ],

  properties: {
    /**
     * Preferences state.
     */
    prefs: {
      type: Object,
      notify: true,
    },

    /** @private */
    pages_: {
      type: Object,
      value: settings.PageStatus,
      readOnly: true,
    },

    /**
     * Current page status
     * 'configure' | 'setup' | 'spinner'
     * @private
     */
    pageStatus_: {
      type: String,
      value: 'configure',
    },

    /**
     * Dictionary defining page visibility.
     * @type {!PrivacyPageVisibility}
     */
    pageVisibility: Object,

    /**
     * The current sync preferences, supplied by SyncBrowserProxy.
     * @type {settings.SyncPrefs|undefined}
     */
    syncPrefs: {
      type: Object,
    },

    /** @type {settings.SyncStatus} */
    syncStatus: {
      type: Object,
    },

    /**
     * The passphrase is seed encode as bip39 keywords.
     * @private
     */
    passphrase_: {
      type: String,
      value: '',
    },

    /** @private */
    syncDisabledByAdmin_: {
      type: Boolean,
      value: false,
      computed: 'computeSyncDisabledByAdmin_(syncStatus.managed)',
    },

    /** @private */
    syncSectionDisabled_: {
      type: Boolean,
      value: false,
      computed: 'computeSyncSectionDisabled_(' +
          'syncStatus.disabled, ' +
          'syncStatus.hasError, syncStatus.statusAction, ' +
          'syncPrefs.trustedVaultKeysRequired)',
    },
  },

  observers: [
    'updatePageStatus_(syncStatus.*)'
  ],

  /** @private {?settings.SyncBrowserProxy} */
  browserProxy_: null,

  /**
   * The beforeunload callback is used to show the 'Leave site' dialog. This
   * makes sure that the user has the chance to go back and confirm the sync
   * opt-in before leaving.
   *
   * This property is non-null if the user is currently navigated on the sync
   * settings route.
   *
   * @private {?Function}
   */
  beforeunloadCallback_: null,

  /**
   * The unload callback is used to cancel the sync setup when the user hits
   * the browser back button after arriving on the page.
   * Note: Cases like closing the tab or reloading don't need to be handled,
   * because they are already caught in |PeopleHandler::~PeopleHandler|
   * from the C++ code.
   *
   * @private {?Function}
   */
  unloadCallback_: null,

  /**
   * Whether the user completed setup successfully.
   * @private {boolean}
   */
  setupSuccessful_: false,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.SyncBrowserProxyImpl.getInstance();
  },

  /** @override */
  attached: function() {
    this.addWebUIListener(
      'sync-prefs-changed', this.handleSyncPrefsChanged_.bind(this));
    const router = settings.Router.getInstance();
    if (router.getCurrentRoute() == router.getRoutes().BRAVE_SYNC_SETUP) {
      this.onNavigateToPage_();
    }
  },

  /** @override */
  detached: function() {
    const router = settings.Router.getInstance();
    if (router.getRoutes().BRAVE_SYNC_SETUP.contains(router.getCurrentRoute())) {
      this.onNavigateAwayFromPage_();
    }

    if (this.beforeunloadCallback_) {
      window.removeEventListener('beforeunload', this.beforeunloadCallback_);
      this.beforeunloadCallback_ = null;
    }
    if (this.unloadCallback_) {
      window.removeEventListener('unload', this.unloadCallback_);
      this.unloadCallback_ = null;
    }
  },

  updatePageStatus_: function () {
    const isFirstSetup = this.syncStatus && this.syncStatus.firstSetupInProgress
    this.pageStatus_ = isFirstSetup ? 'setup' : 'configure'
  },

  /**
   * @return {boolean}
   * @private
   */
  computeSyncSectionDisabled_: function() {
    return this.syncStatus !== undefined &&
        (!!this.syncStatus.disabled ||
         (!!this.syncStatus.hasError &&
          this.syncStatus.statusAction !==
              settings.StatusAction.ENTER_PASSPHRASE &&
          this.syncStatus.statusAction !==
              settings.StatusAction.RETRIEVE_TRUSTED_VAULT_KEYS));
  },

  /**
   * @return {boolean}
   * @private
   */
  computeSyncDisabledByAdmin_() {
    return this.syncStatus != undefined && !!this.syncStatus.managed;
  },

  /** @protected */
  currentRouteChanged: function() {
    const router = settings.Router.getInstance();
    if (router.getCurrentRoute() == router.getRoutes().BRAVE_SYNC_SETUP) {
      this.onNavigateToPage_();
      return;
    }

    if (router.getRoutes().BRAVE_SYNC_SETUP.contains(router.getCurrentRoute())) {
      return;
    }

    this.onNavigateAwayFromPage_();
  },

  /**
   * @param {!settings.PageStatus} expectedPageStatus
   * @return {boolean}
   * @private
   */
  isStatus_: function(expectedPageStatus) {
    return expectedPageStatus == this.pageStatus_;
  },

  /** @private */
  onNavigateToPage_: function() {
    const router = settings.Router.getInstance();
    assert(router.getCurrentRoute() == router.getRoutes().BRAVE_SYNC_SETUP);
    if (this.beforeunloadCallback_) {
      return;
    }

    // Triggers push of prefs to our handler
    this.browserProxy_.didNavigateToSyncPage();

    this.beforeunloadCallback_ = event => {
      // When the user tries to leave the sync setup, show the 'Leave site'
      // dialog.
      if (this.syncStatus && this.syncStatus.firstSetupInProgress) {
        event.preventDefault();
        event.returnValue = '';

        chrome.metricsPrivate.recordUserAction(
            'Signin_Signin_AbortAdvancedSyncSettings');
      }
    };
    window.addEventListener('beforeunload', this.beforeunloadCallback_);

    this.unloadCallback_ = this.onNavigateAwayFromPage_.bind(this);
    window.addEventListener('unload', this.unloadCallback_);
  },

  /** @private */
  onNavigateAwayFromPage_: function() {
    if (!this.beforeunloadCallback_) {
      return;
    }

    this.browserProxy_.didNavigateAwayFromSyncPage(!this.setupSuccessful_);

    window.removeEventListener('beforeunload', this.beforeunloadCallback_);
    this.beforeunloadCallback_ = null;

    if (this.unloadCallback_) {
      window.removeEventListener('unload', this.unloadCallback_);
      this.unloadCallback_ = null;
    }
  },

  /**
   * Handler for when the sync preferences are updated.
   * @private
   */
  handleSyncPrefsChanged_: async function(syncPrefs) {
    this.syncPrefs = syncPrefs;
    // Enforce encryption
    if (this.syncStatus && !this.syncStatus.firstSetupInProgress) {
      if (!this.syncPrefs.encryptAllData) {
        this.syncPrefs.encryptAllData = true;
        this.syncPrefs.setNewPassphrase = true;
        this.syncPrefs.passphrase = this.passphrase_;
        await this.browserProxy_.setSyncEncryption(this.syncPrefs)
      } else if (this.syncPrefs.passphraseRequired) {
        this.syncPrefs.setNewPassphrase = false;
        this.syncPrefs.passphrase = this.passphrase_;
        await this.browserProxy_.setSyncEncryption(this.syncPrefs)
      }
    }
  },

  /**
   * Called when setup is complete and sync code is set
   * @private
   */
  onSetupSuccess_: function() {
    this.setupSuccessful_ = true
    // This navigation causes the firstSetupInProgress flag to be marked as false
    // via `didNavigateAwayFromSyncPage`.
    const router = settings.Router.getInstance();
    if (router.getCurrentRoute() == router.getRoutes().BRAVE_SYNC_SETUP) {
      router.navigateTo(router.getRoutes().BRAVE_SYNC);
    }
  },
});

})();
