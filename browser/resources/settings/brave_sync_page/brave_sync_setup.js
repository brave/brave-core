// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * @fileoverview
 * 'brave-sync-setup' is the UI for starting or joining a sync chain
 * settings.
 */
Polymer({
  is: 'settings-brave-sync-setup',

  properties: {
    syncCode: {
      type: String,
      notify: true
    },
     /**
     * Sync code dialog type. Can only have 1 at a time, so use a single property.
     * 'qr' | 'words' | 'input' | 'choose' | null
     * @private
     */
    syncCodeDialogType_: String,
    isSubmittingSyncCode_: {
      type: Boolean,
      value: false,
    },
    isGettingSyncCode_: {
      type: Boolean,
      value: false,
    },
    isInvalidSyncCode_: {
      type: Boolean,
      value: false,
    }
  },

  /** @private {?settings.BraveSyncBrowserProxy} */
  syncBrowserProxy_: null,

  created: function() {
    this.syncBrowserProxy_ = settings.BraveSyncBrowserProxy.getInstance();
  },

  handleStartSyncChain_: async function () {
    this.isGettingSyncCode_ = true
    const syncCode = await this.syncBrowserProxy_.getSyncCode()
    this.isGettingSyncCode_ = false
    this.syncCode = syncCode;
    this.syncCodeDialogType_ = 'choose'
  },

  handleJoinSyncChain_: function () {
    this.syncCode = undefined
    this.syncCodeDialogType_ = 'input'
  },

  handleSyncCodeDialogDone_: function (e) {
    this.submitSyncCode_()
  },

  submitSyncCode_: async function () {
    this.isSubmittingSyncCode_ = true
    const syncCodeToSubmit = this.syncCode || ''
    let success = false
    try {
      success = await this.syncBrowserProxy_.setSyncCode(syncCodeToSubmit)
    } catch (e) {
      console.error("Error setting sync code")
      success = false
    }
    this.isSubmittingSyncCode_ = false
    if (!success) {
      this.isInvalidSyncCode_ = true
    } else {
      this.syncCodeDialogType_ = undefined
      this.fire('setup-success')
    }
  },
});
