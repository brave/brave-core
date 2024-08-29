// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js';

import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {BaseMixin} from '../base_mixin.js'
import {BraveSyncBrowserProxy} from './brave_sync_browser_proxy.js';
import {getTemplate} from './brave_sync_code_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-sync-code-dialog' contains the dialog for displaying or
 * inputting the sync code. It can display in input mode or in read-only
 * words, qr-code, or a 'chooser' which will allow the user to choose words or
 * qr-code.
 */

const SettingsBraveSyncCodeDialogElementBase =
  I18nMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & I18nMixinInterface
  }

export class SettingsBraveSyncCodeDialogElement extends SettingsBraveSyncCodeDialogElementBase {
  static get is() {
    return 'settings-brave-sync-code-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
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
        value: 0
      },
      hasCopiedSyncCode_: {
        type: Boolean,
        value: false
      },
      qrCodeImageUrl_: {
        type: String,
        value: null
      },
    };
  }

  static get observers() {
    return [
      'getQRCode_(syncCode, codeType)',
      'computeSyncCodeWordCount_(syncCode, codeType)',
    ];
  }

  private syncCode: string;
  private codeType: 'qr' | 'words' | 'choose' | 'input' | null;
  private syncCodeValidationError: string;
  private syncCodeWordCount_: number;
  private hasCopiedSyncCode_: boolean;
  private qrCodeImageUrl_: string;
  private hasCopiedSyncCodeTimer_: ReturnType<typeof window.setTimeout>;

  syncBrowserProxy_: BraveSyncBrowserProxy = BraveSyncBrowserProxy.getInstance();

  async computeSyncCodeWordCount_() {
    if (this.codeType !== 'input') {
      return
    }

    if (!this.syncCode) {
      return
    }

    this.syncCodeWordCount_ =
      await this.syncBrowserProxy_.getWordsCount(this.syncCode)
  }

  isCodeType(askingType: string) {
    return (this.codeType === askingType)
  }

  handleClose_() {
    this.codeType = null
    this.syncCodeValidationError = ''
  }

  handleChooseMobile_() {
    this.codeType = null
    window.setTimeout(() => {
      this.codeType = 'qr'
    }, 0)
  }

  handleChooseComputer_() {
    this.codeType = null
    window.setTimeout(() => {
      this.codeType = 'words'
    }, 0)
  }

  handleSyncCodeCopy_() {
    window.clearTimeout(this.hasCopiedSyncCodeTimer_)
    navigator.clipboard.writeText(this.syncCode)
    this.hasCopiedSyncCode_ = true
    this.hasCopiedSyncCodeTimer_ = window.setTimeout(() => {
      this.hasCopiedSyncCode_ = false
    }, 4000)
  }

  handleDone_() {
    this.dispatchEvent(new CustomEvent('done'))
  }

  async getQRCode_() {
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
  }
}

customElements.define(
  SettingsBraveSyncCodeDialogElement.is, SettingsBraveSyncCodeDialogElement)
