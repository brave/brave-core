// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

 (function() {

  /**
   * @fileoverview
   * 'settings-sync-page' is the settings page containing sync settings.
   */
  Polymer({
    is: 'settings-brave-sync-code-dialog',

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
      isInvalidSyncCode: {
        type: Boolean,
        value: false,
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
    },

    observers: [
      'updateSyncCodeValidity_(syncCode)',
      'getQRCode_(syncCode, codeType)',
    ],

    attached: function() {
    },

    updateSyncCodeValidity_: function() {
      this.isInvalidSyncCode = false
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
    },

    handleChooseMobile_: function() {
      this.codeType = 'qr'
    },

    handleChooseComputer_: function() {
      this.codeType = 'words'
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
      if (!this.syncCode || this.codeType !== 'qr') {
        return
      }
      const data = await cr.sendWithPromise('SyncGetQRCode', this.syncCode)
      console.log('qr code', data)
      this.$$('#qrCode').innerText = data
      // TODO(petemill): generate a canvas / image
    },

  });

  })();
