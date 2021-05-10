// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.


import 'chrome://resources/js/util.m.js';

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.m.js';
import 'chrome://resources/cr_elements/icons.m.js';
import 'chrome://resources/cr_elements/shared_style_css.m.js';
import 'chrome://resources/cr_elements/shared_vars_css.m.js';
import '../settings_shared_css.js';
import '../settings_vars_css.js';
import '../people_page/sync_controls.js';
import './brave_sync_configure.js';
import './brave_sync_setup.js';

import {assert} from 'chrome://resources/js/assert.m.js';
import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js';

import {Route, RouteObserverBehavior, Router} from '../router.js';
import {SyncBrowserProxyImpl, StatusAction} from '../people_page/sync_browser_proxy.js';

/**
* @fileoverview
* 'settings-sync-subpage' is the settings page content
*/
Polymer({
  is: 'settings-brave-sync-subpage',

  _template: html`{__html_template__}`,

  behaviors: [
    RouteObserverBehavior,
  ],

  properties: {
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
    * The current sync preferences, supplied by SyncBrowserProxy.
    * @type {SyncPrefs|undefined}
    */
    syncPrefs: {
      type: Object,
    },

    /** @type {SyncStatus} */
    syncStatus: {
      type: Object,
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

  /** @private {?SyncBrowserProxy} */
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
    this.browserProxy_ = SyncBrowserProxyImpl.getInstance();
  },

  /** @override */
  attached: function() {
    const router = Router.getInstance();
    if (router.getCurrentRoute() == router.getRoutes().BRAVE_SYNC_SETUP) {
      this.onNavigateToPage_();
    }
  },

  /** @override */
  detached: function() {
    const router = Router.getInstance();
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
              StatusAction.ENTER_PASSPHRASE &&
          this.syncStatus.statusAction !==
              StatusAction.RETRIEVE_TRUSTED_VAULT_KEYS));
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
    const router = Router.getInstance();
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
  * @param {!PageStatus} expectedPageStatus
  * @return {boolean}
  * @private
  */
  isStatus_: function(expectedPageStatus) {
    return expectedPageStatus == this.pageStatus_;
  },

  /** @private */
  onNavigateToPage_: function() {
    const router = Router.getInstance();
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

    // Reset state as this component could actually be kept around even though
    // it is hidden.
    this.setupSuccessful_ = false

    window.removeEventListener('beforeunload', this.beforeunloadCallback_);
    this.beforeunloadCallback_ = null;

    if (this.unloadCallback_) {
      window.removeEventListener('unload', this.unloadCallback_);
      this.unloadCallback_ = null;
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
    const router = Router.getInstance();
    if (router.getCurrentRoute() == router.getRoutes().BRAVE_SYNC_SETUP) {
      router.navigateTo(router.getRoutes().BRAVE_SYNC);
    }
  },
});
