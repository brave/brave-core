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
     * The current page status. Defaults to |CONFIGURE| such that the searching
     * algorithm can search useful content when the page is not visible to the
     * user.
     * @private {?settings.PageStatus}
     */
    pageStatus_: {
      type: String,
      value: settings.PageStatus.CONFIGURE,
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
    syncSectionDisabled_: {
      type: Boolean,
      value: false,
      computed: 'computeSyncSectionDisabled_(' +
          'syncStatus.disabled, ' +
          'syncStatus.hasError, syncStatus.statusAction, ' +
          'syncPrefs.trustedVaultKeysRequired)',
    },

    /** @private */
    showSetupCancelDialog_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showInvalidSyncCodeDialog_: {
      type: Boolean,
      value: false,
    },
  },

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
   * Whether the user decided to abort sync.
   * @private {boolean}
   */
  didAbort_: true,

  /**
   * Whether the user confirmed the cancellation of sync.
   * @private {boolean}
   */
  setupCancelConfirmed_: false,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.SyncBrowserProxyImpl.getInstance();
  },

  /** @override */
  attached: function() {
    this.addWebUIListener(
        'page-status-changed', this.handlePageStatusChanged_.bind(this));
    this.addWebUIListener(
        'sync-prefs-changed', this.handleSyncPrefsChanged_.bind(this));

    if (settings.getCurrentRoute() == settings.routes.BRAVE_SYNC_SETUP) {
      this.onNavigateToPage_();
    }
  },

  /** @override */
  detached: function() {
    if (settings.routes.BRAVE_SYNC_SETUP.contains(settings.getCurrentRoute())) {
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

  /**
   * Handler for when the sync state is pushed from the browser.
   * @param {?settings.SyncStatus} syncStatus
   * @private
   */
  handleSyncStatus_: function(syncStatus) {
    this.syncStatus = syncStatus;
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

  /** @private */
  onSetupCancelDialogBack_: function() {
    this.$$('#setupCancelDialog').cancel();
    chrome.metricsPrivate.recordUserAction(
        'Signin_Signin_CancelCancelAdvancedSyncSettings');
  },

  /** @private */
  onSetupCancelDialogConfirm_: function() {
    this.setupCancelConfirmed_ = true;
    this.$$('#setupCancelDialog').close();
    settings.navigateTo(settings.routes.BRAVE_SYNC);
    chrome.metricsPrivate.recordUserAction(
        'Signin_Signin_ConfirmCancelAdvancedSyncSettings');
  },

  /** @private */
  onSetupCancelDialogClose_: function() {
    this.showSetupCancelDialog_ = false;
  },

  /** @private */
  onInvalidSyncCodeDialogClose_: function() {
    this.showInvalidSyncCodeDialog_ = false;
  },

  /** @private */
  onInvalidSyncCodeDialogConfirm_: function() {
    this.$$('#invalidSyncCodeDialog').close();
  },

  /** @protected */
  currentRouteChanged: function() {
    if (settings.getCurrentRoute() == settings.routes.BRAVE_SYNC_SETUP) {
      this.onNavigateToPage_();
      return;
    }

    if (settings.routes.BRAVE_SYNC_SETUP.contains(settings.getCurrentRoute())) {
      return;
    }

    const searchParams = settings.getQueryParameters().get('search');
    if (searchParams) {
      // User navigated away via searching. Cancel sync without showing
      // confirmation dialog.
      this.onNavigateAwayFromPage_();
      return;
    }

    const userActionCancelsSetup = this.syncStatus &&
        this.syncStatus.firstSetupInProgress && this.didAbort_;
    if (userActionCancelsSetup && !this.setupCancelConfirmed_) {
      chrome.metricsPrivate.recordUserAction(
          'Signin_Signin_BackOnAdvancedSyncSettings');
      // Show the 'Cancel sync?' dialog.
      // Yield so that other |currentRouteChanged| observers are called,
      // before triggering another navigation (and another round of observers
      // firing). Triggering navigation from within an observer leads to some
      // undefined behavior and runtime errors.
      requestAnimationFrame(() => {
        settings.navigateTo(settings.routes.BRAVE_SYNC_SETUP);
        this.showSetupCancelDialog_ = true;
        // Flush to make sure that the setup cancel dialog is attached.
        Polymer.dom.flush();
        this.$$('#setupCancelDialog').showModal();
      });
      return;
    }

    // Reset variable.
    this.setupCancelConfirmed_ = false;

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
    assert(settings.getCurrentRoute() == settings.routes.BRAVE_SYNC_SETUP);

    this.browserProxy_.getSyncStatus().then(
        this.handleSyncStatus_.bind(this));

    if (this.beforeunloadCallback_) {
      return;
    }

    // Display loading page until the settings have been retrieved.
    this.pageStatus_ = settings.PageStatus.SPINNER;

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

    // Reset the status to CONFIGURE such that the searching algorithm can
    // search useful content when the page is not visible to the user.
    this.pageStatus_ = settings.PageStatus.CONFIGURE;

    this.browserProxy_.didNavigateAwayFromSyncPage(this.didAbort_);

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
  handleSyncPrefsChanged_: function(syncPrefs) {
    this.syncPrefs = syncPrefs;
    this.pageStatus_ = settings.PageStatus.CONFIGURE;
  },

  /**
   * Called when the page status updates.
   * @param {!settings.PageStatus} pageStatus
   * @private
   */
  handlePageStatusChanged_: function(pageStatus) {
    switch (pageStatus) {
      case settings.PageStatus.SPINNER:
      case settings.PageStatus.TIMEOUT:
      case settings.PageStatus.CONFIGURE:
        this.pageStatus_ = pageStatus;
        return;
      case settings.PageStatus.DONE:
        if (settings.getCurrentRoute() == settings.routes.BRAVE_SYNC_SETUP) {
          settings.navigateTo(settings.routes.BRAVE_SYNC);
        }
        return;
      case settings.PageStatus.PASSPHRASE_FAILED:
        if (this.pageStatus_ == this.pages_.CONFIGURE && this.syncPrefs &&
            this.syncPrefs.passphraseRequired) {
          // This won't happen because client need valid seed/passphrase to be
          // able to get data from server.
        }
        return;
    }

    assertNotReached();
  },

  /**
   * @return {boolean}
   * @private
   */
  shouldShowSyncControl_: function() {
    return this.syncStatus !== undefined &&
        !!this.syncStatus.syncSystemEnabled;
  },

  /**
   * @param {!CustomEvent<boolean>} e The event passed from
   *     settings-sync-control.
   * @private
   */
  onSyncSetupDone_: function(e) {
    if (e.detail) {
      this.browserProxy_.setSyncCode(this.passphrase_).then((success) => {
        if (success) {
          this.didAbort_ = false;
          settings.navigateTo(settings.routes.BRAVE_SYNC);
        } else {
          this.passphrase_ = '';
          this.showInvalidSyncCodeDialog_ = true;
          Polymer.dom.flush();
          this.$$('#invalidSyncCodeDialog').showModal();
        }
      });
    } else {
      settings.navigateTo(settings.routes.BRAVE_SYNC);
    }
  },
});

})();
