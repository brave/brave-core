/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

 import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
 import { getCss } from './brave_account_forgot_password_dialog.css.js'
 import { getHtml } from './brave_account_forgot_password_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-forgot-password-dialog'...
 */

export interface SettingsBraveAccountForgotPasswordDialogElement {
  $: {
    email: HTMLInputElement,
  }
}

export class SettingsBraveAccountForgotPasswordDialogElement extends CrLitElement {
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
      isEmailValid: { type: Boolean },
    }
  }

  protected onEmailInput() {
    // https://www.regular-expressions.info
    this.isEmailValid = /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}$/.test(this.$.email.value)
  }

  protected isEmailValid: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-forgot-password-dialog': SettingsBraveAccountForgotPasswordDialogElement
  }
}

customElements.define(
  SettingsBraveAccountForgotPasswordDialogElement.is, SettingsBraveAccountForgotPasswordDialogElement)
