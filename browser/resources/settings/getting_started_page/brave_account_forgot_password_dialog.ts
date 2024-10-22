/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js';
import 'chrome://resources/brave/leo.bundle.js'

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {getTemplate} from './brave_account_forgot_password_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-forgot-password-dialog'...
 */

interface SettingsBraveAccountForgotPasswordDialogElement {
  $: {
    email: HTMLInputElement,
  };
}

class SettingsBraveAccountForgotPasswordDialogElement extends PolymerElement {
  static get is() {
    return 'settings-brave-account-forgot-password-dialog'
  }

  static get template() {
    return getTemplate()
  }

  private onEmailInput() {
    // https://www.regular-expressions.info
    this.isEmailValid = Boolean(this.$.email.value.match('^[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,4}$'))
  }

  private onCancelButtonClicked() {
    this.dispatchEvent(new CustomEvent('cancel-button-clicked'))
  }

  private isEmailValid: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-forgot-password-dialog': SettingsBraveAccountForgotPasswordDialogElement;
  }
}

customElements.define(
  SettingsBraveAccountForgotPasswordDialogElement.is, SettingsBraveAccountForgotPasswordDialogElement)
