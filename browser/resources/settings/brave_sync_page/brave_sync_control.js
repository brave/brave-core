// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @fileoverview
 * 'settings-sync-account-section' is the settings page containing sign-in
 * settings.
 */
cr.exportPath('settings');

Polymer({
  is: 'settings-brave-sync-control',

  behaviors: [
    WebUIListenerBehavior,
    PrefsBehavior,
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

    /**
     * The current sync status, supplied by parent element.
     * @type {!settings.SyncStatus}
     */
    syncStatus: Object,

    /** @private */
    showSetupDialog_: {
      type: Boolean,
      computed: 'computeShowSetupDialog_(' +
      'syncStatus.firstSetupInProgress)',
    },

    passphrase: {
      type: String,
      value: '',
      notify: true,
    },

    /** @private */
    showSyncSetupDialog_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showDeviceTypeDialog_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showQRCodeDialog_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showSyncCodeDialog_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showHaveSyncCodeDialog_: {
      type: Boolean,
      value: false,
    },
  },

  /** @private {?settings.SyncBrowserProxy} */
  syncBrowserProxy_: null,

  created: function() {
    this.syncBrowserProxy_ = settings.SyncBrowserProxyImpl.getInstance();
  },

  attached: function() {
    // TODO(darkdh): need a way to recreate this component, otherwise nagigate
    // out of subpage and back will not show correct state
    Polymer.dom.flush();
    this.$$('#syncSetup').showModal();
  },

  /** @protected */
  currentRouteChanged: function() {
    if (settings.getCurrentRoute() != settings.routes.BRAVE_SYNC_SETUP) {
      this.showSyncCodeDialog_ = false;
      this.showHaveSyncCodeDialog_ = false;
      this.showDeviceTypeDialog_ = false;
      this.showQRCodeDialog_ = false;
    }
  },

  /**
   * @return {boolean}
   * @private
   */
  computeShowSetupDialog_: function() {
    return !!this.syncStatus.firstSetupInProgress;
  },

  /** @private */
  onSetupCancel_: function() {
    this.fire('sync-setup-done', false);
  },

  /** @private */
  onSetupConfirm_: function() {
    if (!this.passphrase) {
      this.passphrase = this.$$('#enterSyncCode').value;
    }
    this.fire('sync-setup-done', true);
  },

  /** @private */
  onViewQRCode_: function() {
    this.syncBrowserProxy_.getSyncCode().then((syncCode) => {
      this.passphrase = syncCode;
    })
    this.showDeviceTypeDialog_ = false;
    this.showQRCodeDialog_ = true;
    Polymer.dom.flush();
    this.$$('#QRCode').showModal();
  },

  /** @private */
  onQRCodeCancel_: function() {
    this.showQRCodeDialog_ = false;
  },

  /** @private */
  onViewSyncCode_: function() {
    this.syncBrowserProxy_.getSyncCode().then((syncCode) => {
      this.passphrase = syncCode;
    })
    this.showDeviceTypeDialog_ = false;
    this.showQRCodeDialog_ = false;
    this.showSyncCodeDialog_ = true;
    Polymer.dom.flush();
    this.$$('#syncCodeDialog').showModal();
  },

  /** @private */
  onSyncCodeDialogClose_: function() {
    this.showSyncCodeDialog_ = false;
  },

  /** @private */
  onDeviceTypeDialogClose_: function() {
    this.showDeviceTypeDialog_ = false;
  },

  /** @private */
  onSyncSetupDialogClose_: function() {
    if (!this.passphrase) {
      this.onSetupCancel_();
    } else {
      this.onSetupConfirm_();
    }
  },

  /** @private */
  onHaveSyncCodeDialogClose_: function() {
    this.showHaveSyncCodeDialog_ = false;
  },

  /** @private */
  onSyncCodeCopy_: function() {
    navigator.clipboard.writeText(this.$$('#syncCode').value);
  },

  /** @private */
  onStartANewSyncChain_: function() {
    this.showDeviceTypeDialog_ = true;
    Polymer.dom.flush();
    this.$$('#deviceType').showModal();
  },

  /** @private */
  onHaveSyncCode_: function() {
    this.showHaveSyncCodeDialog_ = true;
    Polymer.dom.flush();
    this.$$('#haveSyncCode').showModal();
  },
});
