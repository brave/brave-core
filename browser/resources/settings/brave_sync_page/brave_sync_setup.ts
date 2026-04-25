// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import './brave_sync_code_dialog.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {BaseMixin} from '../base_mixin.js'
import {BraveSyncBrowserProxy} from './brave_sync_browser_proxy.js';
import {getTemplate} from './brave_sync_setup.html.js'

/**
 * @fileoverview
 * 'brave-sync-setup' is the UI for starting or joining a sync chain
 * settings.
 */

const SettingsBraveSyncSetupElementBase =
  I18nMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & I18nMixinInterface
  }

export class SettingsBraveSyncSetupElement extends SettingsBraveSyncSetupElementBase {
  static get is() {
    return 'settings-brave-sync-setup'
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
      * Sync code dialog type. Can only have 1 at a time, so use a single property.
      * 'qr' | 'words' | 'input' | 'choose' | null
      */
      syncCodeDialogType: {
        type: String,
        notify: true,
      },
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
    };
  }

  private declare syncCode: string | undefined;
  private declare syncCodeDialogType: 'qr' | 'words' | 'input' | 'choose' | null;
  private declare isSubmittingSyncCode_: boolean;
  private declare isGettingSyncCode_: boolean;
  private declare syncCodeValidationError_: string;

  syncBrowserProxy_: BraveSyncBrowserProxy = BraveSyncBrowserProxy.getInstance();

  async handleStartSyncChain_() {
    this.isGettingSyncCode_ = true
    const syncCode = await this.syncBrowserProxy_.getSyncCode()
    this.isGettingSyncCode_ = false
    this.syncCode = syncCode;
    this.syncCodeDialogType = 'choose'
  }

  handleJoinSyncChain_() {
    this.syncCode = undefined
    this.syncCodeDialogType = 'input'
  }

  handleSyncCodeDialogDone_() {
    if (this.syncCodeDialogType === 'input') {
      const messageText = this.i18n('braveSyncFinalSecurityWarning')
      const shouldProceed = confirm(messageText)
      if (!shouldProceed) {
        return
      }
    }

    this.submitSyncCode_()
  }

  async submitSyncCode_() {
    this.isSubmittingSyncCode_ = true
    const syncCodeToSubmit = this.syncCode || ''
    let success = false
    try {
      success = await this.syncBrowserProxy_.setSyncCode(syncCodeToSubmit)
    } catch (e: unknown) {
      this.syncCodeValidationError_ = (e as string)
      success = false
    }
    this.isSubmittingSyncCode_ = false
    if (success) {
      this.syncCodeDialogType = null
      this.dispatchEvent(new CustomEvent('setup-success'))
    }
  }
}

customElements.define(
  SettingsBraveSyncSetupElement.is, SettingsBraveSyncSetupElement)
