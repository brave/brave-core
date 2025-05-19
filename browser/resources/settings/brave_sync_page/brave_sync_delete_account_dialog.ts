// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js';

import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {BaseMixin} from '../base_mixin.js'
import {BraveSyncBrowserProxy} from './brave_sync_browser_proxy.js';
import {getTemplate} from './brave_sync_delete_account_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-sync-delete-account-dialog' contains the dialog for
 * displaying a warning message and confirm further action or cancel.
 * It is used for confirmation of permanently delete account.
 */

const SettingsBraveSyncDeleteAccountDialogElementBase =
  I18nMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & I18nMixinInterface
  }

export class SettingsBraveSyncCodeDialogElement
             extends SettingsBraveSyncDeleteAccountDialogElementBase {
  static get is() {
    return 'settings-brave-sync-delete-account-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
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
       * Flag indicating that right now delete account was sent, but response
       * is not yet received. Used to hide controls.
       */
      deleteIsInProgress: {
        type: Boolean,
        value: false,
        notify: true
      },
    };
  }

  private declare deleteAccountError: string;
  private declare deleteIsInProgress: boolean;
  private declare doingDeleteAccount: boolean;

  syncBrowserProxy_: BraveSyncBrowserProxy =
                                            BraveSyncBrowserProxy.getInstance();

  async handleDeleteAccount_() {
    let errorText = ''
    this.deleteIsInProgress = true
    this.deleteAccountError = ''
    try {
      await this.syncBrowserProxy_.permanentlyDeleteSyncAccount()
    } catch (e: unknown) {
      errorText = (e as string)
    }

    this.deleteIsInProgress = false
    this.deleteAccountError = errorText

    if (!this.deleteAccountError) {
      this.doingDeleteAccount = false
    }
  }

  handleCancel_() {
    this.doingDeleteAccount = false
  }
}

customElements.define(
  SettingsBraveSyncCodeDialogElement.is, SettingsBraveSyncCodeDialogElement)
