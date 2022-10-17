// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import '../settings_shared.css.js';

import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';

import {Polymer} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nBehavior} from 'chrome://resources/cr_elements/i18n_behavior.js';
import {BraveSyncBrowserProxy} from './brave_sync_browser_proxy.js';
import {getTemplate} from './brave_sync_delete_account_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-sync-delete-account-dialog' contains the dialog for displaying a
 * warning message and confirm further action or cancel. It is used for
 * confirmation of permanently delete account.
 */

 Polymer({
   is: 'settings-brave-sync-delete-account-dialog',

   _template: getTemplate(),

   behaviors: [
     I18nBehavior
   ],

   properties: {
     /**
      * When true current dialog is displayed
      */
     doingDeleteAccount: {
       type: Boolean,
       notify: true
     },

     /**
      * String representing error of account deletion if any present
      */
     deleteAccountError: {
       type: String,
       value: '',
       notify: true
     },

     /**
      * Flag indicating that right now delete account was sent, but response not 
      * yet received. Used to hide controls.
      */
     deleteIsInProgress: {
       type: Boolean,
       value: false,
       notify: true
     },
  },

  /** @private {?SyncBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = BraveSyncBrowserProxy.getInstance()
  },

  handleDeleteAccount_: async function() {
    let error_text = ''
    this.deleteIsInProgress = true
    this.deleteAccountError = ''
    try {
      await this.browserProxy_.permanentlyDeleteSyncAccount()
    } catch (e) {
      error_text = e
    }

    this.deleteIsInProgress = false
    this.deleteAccountError = error_text

    if (!this.deleteAccountError) {
      this.doingDeleteAccount = false
    }
  },

  handleCancel_: function() {
    this.doingDeleteAccount = false
  },
});
