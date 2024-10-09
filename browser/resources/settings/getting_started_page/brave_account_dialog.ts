// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {BaseMixin} from '../base_mixin.js'
import {getTemplate} from './brave_account_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-dialog'...
 */

const SettingsBraveAccountDialogElementBase =
  I18nMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & I18nMixinInterface
  }

export class SettingsBraveAccountDialogElement extends SettingsBraveAccountDialogElementBase {
  static get is() {
    return 'settings-brave-account-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      /**
       * 'create' | 'signin'
       */
      codeType: {
        type: String,
        value: 'signin',
        notify: true
      },
      isTermsAccepted_: {
        type: Boolean,
        value: false,
      },
      isEmailAddressInvalid_: {
        type: Boolean,
        value: true,
      }
    };
  }

  protected onConditionsChanged_() {
    this.canCreateAccount_ = this.isTermsAccepted_ && !this.isEmailAddressInvalid_;
  }

  private codeType: 'create' | 'signin' | null;
  private isTermsAccepted_: boolean = false;
  private isEmailAddressInvalid_: boolean = true;
  private canCreateAccount_: boolean = false;

  isCodeType(askingType: string) {
    return (this.codeType === askingType)
  }

  handleClose_() {
    this.codeType = null
  }
}

customElements.define(
  SettingsBraveAccountDialogElement.is, SettingsBraveAccountDialogElement)
