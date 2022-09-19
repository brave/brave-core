// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * @fileoverview
 * 'brave-sync-setup' is the UI for starting or joining a sync chain
 * settings.
 */
import './brave_sync_code_dialog.js';

import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nBehavior} from 'chrome://resources/cr_elements/i18n_behavior.js';

import {BraveSyncBrowserProxy} from './brave_sync_browser_proxy.js';

Polymer({
  is: 'settings-brave-sync-setup',

  _template: html`{__html_template__}`,

  behaviors: [
    I18nBehavior,
  ],

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
    syncCodeValidationError_: {
      type: String,
      value: '',
     }
  },

  /** @private {?BraveSyncBrowserProxy} */
  syncBrowserProxy_: null,

  created: function() {
    this.syncBrowserProxy_ = BraveSyncBrowserProxy.getInstance();
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
    if (this.syncCodeDialogType_ === 'input') {
      const messageText = this.i18n('braveSyncFinalSecurityWarning')
      const shouldProceed = confirm(messageText)
      if (!shouldProceed) {
        return
      }
    }

    this.submitSyncCode_()
  },

  submitSyncCode_: async function () {
    this.isSubmittingSyncCode_ = true
    const syncCodeToSubmit = this.syncCode || ''
    let success = false
    try {
      success = await this.syncBrowserProxy_.setSyncCode(syncCodeToSubmit)
    } catch (e) {
      this.syncCodeValidationError_ = e
      success = false
    }
    this.isSubmittingSyncCode_ = false
    if (success) {
      this.syncCodeDialogType_ = undefined
      this.fire('setup-success')
    }
  },
});
