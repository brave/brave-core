/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { getCss } from './brave_account_sign_in_dialog.css.js'
import { getHtml } from './brave_account_sign_in_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-sign-in-dialog'...
 */

export class SettingsBraveAccountSignInDialogElement extends CrLitElement {
  static get is() {
    return 'settings-brave-account-sign-in-dialog'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      isEmailAddressValid: { type: Boolean },
      isPasswordValid: { type: Boolean },
    }
  }

  protected onEmailAddressInput(detail: { value: string }) {
    this.isEmailAddressValid =
      /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}$/.test(detail.value)
  }

  protected onPasswordInput(detail: { value: string }) {
    this.isPasswordValid = detail.value.length !== 0
  }

  protected OnEyeIconClicked(event: Event) {
    event.preventDefault()
    const target = event.target as Element
    const isShowing = target.getAttribute('name') === 'eye-on'
    target.setAttribute('name', isShowing ? 'eye-off' : 'eye-on')
    target.parentElement!.setAttribute('type', isShowing ? 'password' : 'text')
  }

  protected isEmailAddressValid: boolean = false
  protected isPasswordValid: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-sign-in-dialog':
    SettingsBraveAccountSignInDialogElement
  }
}

customElements.define(
  SettingsBraveAccountSignInDialogElement.is,
  SettingsBraveAccountSignInDialogElement
)
