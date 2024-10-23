/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { getCss } from './brave_account_sign_in_dialog.css.js'
import { getHtml } from './brave_account_sign_in_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-sign-in-dialog'...
 */

export interface SettingsBraveAccountSignInDialogElement {
  $: {
    email: HTMLInputElement,
    password: HTMLInputElement,
  }
}

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
      isEmailValid: { type: Boolean },
      isPasswordValid: { type: Boolean },
    }
  }

  protected onEmailInput() {
    // https://www.regular-expressions.info
    this.isEmailValid = Boolean(this.$.email.value.match('^[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,4}$'))
  }

  protected onPasswordInput() {
    this.isPasswordValid = this.$.password.value.length !== 0
  }

  protected show(event: Event) {
    event.preventDefault()
    const icon = this.$.password.querySelector('#icon')
    const isShowing = icon!.getAttribute('name') === 'eye-on'
    icon!.setAttribute('name', isShowing ? 'eye-off' : 'eye-on')
    this.$.password.setAttribute('type', isShowing ? 'password' : 'text')
  }

  protected isEmailValid: boolean = false
  protected isPasswordValid: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-sign-in-dialog': SettingsBraveAccountSignInDialogElement
  }
}

customElements.define(
  SettingsBraveAccountSignInDialogElement.is, SettingsBraveAccountSignInDialogElement)
