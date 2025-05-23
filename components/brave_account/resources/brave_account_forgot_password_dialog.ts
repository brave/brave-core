/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { getCss } from './brave_account_forgot_password_dialog.css.js'
import { getHtml } from './brave_account_forgot_password_dialog.html.js'
import { isEmailValid } from './brave_account_common.js'

export class SettingsBraveAccountForgotPasswordDialogElement
  extends CrLitElement {
  static get is() {
    return 'settings-brave-account-forgot-password-dialog'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      email: { type: String },
      isEmailValid: { type: Boolean },
    }
  }

  protected onEmailInput(detail: { value: string }) {
    this.email = detail.value
    this.isEmailValid = isEmailValid(this.email)
  }

  protected email: string = ''
  protected isEmailValid: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-forgot-password-dialog':
    SettingsBraveAccountForgotPasswordDialogElement
  }
}

customElements.define(
  SettingsBraveAccountForgotPasswordDialogElement.is,
  SettingsBraveAccountForgotPasswordDialogElement
)
