// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js';

import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';

import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nBehavior} from 'chrome://resources/cr_elements/i18n_behavior.js';

import {BraveSyncBrowserProxy} from './brave_sync_browser_proxy.js';


/**
 * @fileoverview
 * 'settings-brave-sync-code-dialog' contains the dialog for displaying or
 * inputting the sync code. It can display in input mode or in read-only
 * words, qr-code, or a 'chooser' which will allow the user to choose words or
 * qr-code.
 */
Polymer({
  is: 'settings-brave-sync-code-dialog',

  _template: html`{__html_template__}`,

  behaviors: [
    I18nBehavior
  ],

  properties: {
    syncCode: {
      type: String,
      notify: true
    },
    /**
     * 'qr' | 'words' | 'choose' | 'input'
     */
    codeType: {
      type: String,
      value: 'choose',
      notify: true
    },
    syncCodeValidationError: {
      type: String,
      value: '',
      notify: true
    },
    syncCodeWordCount_: {
      type: Number,
      computed: 'computeSyncCodeWordCount_(syncCode)'
    },
    hasCopiedSyncCode_: {
      type: Boolean,
      value: false
    },
    qrCodeImageUrl_: {
      type: String,
      value: null
    },
  },

  observers: [
    'getQRCode_(syncCode, codeType)',
  ],

  /** @private {?BraveSyncBrowserProxy} */
  syncBrowserProxy_: null,

  created: function() {
    this.syncBrowserProxy_ = BraveSyncBrowserProxy.getInstance();
  },

  computeSyncCodeWordCount_: function() {
    if (!this.syncCode) {
      return 0
    }
    return this.syncCode.trim().split(' ').length
  },

  isCodeType: function(askingType) {
    return (this.codeType === askingType)
  },

  handleClose_: function() {
    this.codeType = null
    this.syncCodeValidationError = ''
  },

  handleChooseMobile_: function() {
    this.codeType = null
    window.setTimeout(() => {
      this.codeType = 'qr'
    }, 0)
  },

  handleChooseComputer_: function() {
    this.codeType = null
    window.setTimeout(() => {
      this.codeType = 'words'
    }, 0)
  },

  handleSyncCodeCopy_: function() {
    window.clearTimeout(this.hasCopiedSyncCodeTimer_)
    navigator.clipboard.writeText(this.syncCode)
    this.hasCopiedSyncCode_ = true
    this.hasCopiedSyncCodeTimer_ = window.setTimeout(() => {
      this.hasCopiedSyncCode_ = false
    }, 4000)
  },

  handleDone_: function() {
    this.fire('done')
  },

  getQRCode_: async function() {
    if (this.codeType !== 'qr') {
      return
    }
    if (!this.syncCode) {
      console.warn('Skip getQRCode because code words are empty')
      return
    }
    try {
      this.qrCodeImageUrl_ = await this.syncBrowserProxy_.getQRCode(this.syncCode)
    } catch (e) {
      console.error('getQRCode failed', e)
    }
  },

});
