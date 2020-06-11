// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * @fileoverview
 * 'settings-brave-sync-configure' is the set of controls which fetches, displays
 * and updates the sync configuration.
 */
Polymer({
  is: 'settings-brave-sync-configure',

  behaviors: [
    I18nBehavior,
    WebUIListenerBehavior,
  ],

  properties: {
    /**
     * The current sync status, supplied by parent element.
     * @type {!settings.SyncStatus}
     */
    syncStatus: Object,
    /**
     * Configured sync code
     */
    syncCode: {
      type: String,
      notify: true
    },
    /**
     * List of devices in sync chain
     * @private
     */
    deviceList_: Array,
    /**
     * Sync code dialog type. Can only have 1 at a time, so use a single property.
     * 'qr' | 'words' | 'input' | 'choose' | null
     * @private
     */
    syncCodeDialogType_: String,
  },

  /** @private {?settings.SyncBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.BraveSyncBrowserProxy.getInstance();
  },

  attached: async function() {
    const [syncCode, deviceList] = await Promise.all([
      this.browserProxy_.getSyncCode(),
      this.browserProxy_.getDeviceList()
    ])
    this.syncCode = syncCode
    this.addWebUIListener('device-info-changed', this.handleDeviceInfo_.bind(this))
    this.handleDeviceInfo_(deviceList)
  },

  handleDeviceInfo_: function(deviceList) {
    console.log('got device list', deviceList)
    this.deviceList_ = deviceList
  },

  getDeviceDisplayDate: function(device) {
    const displayDate = new Date(0)
    displayDate.setUTCSeconds(device.lastUpdatedTimestamp)
    return displayDate.toDateString()
  },

  onViewSyncCode_: function() {
    this.syncCodeDialogType_ = 'words'
  },

  onAddDevice_: function() {
    this.syncCodeDialogType_ = 'choose'
  },

  onSyncCodeDialogDone_: function() {
    this.syncCodeDialogType_ = null
  },

  onResetSyncChain_: async function() {
    // TODO(petemill): translate message
    const messageText = this.i18n('braveSyncResetConfirmation')
    const shouldReset = confirm(messageText)
    if (!shouldReset) {
      return
    }
    await this.browserProxy_.resetSyncChain();
    const router = settings.Router.getInstance();
    router.navigateTo(router.getRoutes().BRAVE_SYNC);
  }
});
