/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js';
import 'chrome://resources/brave/leo.bundle.js'

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {getTemplate} from './brave_account_create_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-create-dialog'...
 */

interface SettingsBraveAccountCreateDialogElement {
  $: {
    email_address: HTMLInputElement,
    account_name: HTMLInputElement,
    create_password: HTMLInputElement,
    confirm_password: HTMLInputElement,
  }
}

class SettingsBraveAccountCreateDialogElement extends PolymerElement {
  static get is() {
    return 'settings-brave-account-create-dialog'
  }

  static get template() {
    return getTemplate()
  }

  private onEmailAddressInput() {
    // https://www.regular-expressions.info
    this.isEmailAddressValid = Boolean(this.$.email_address.value.match('^[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,4}$'))
  }

  private onAccountNameInput() {
    this.isAccountNameValid = this.$.account_name.value.length !== 0
  }

  private onCreatePasswordInput() {
    this.isCreatePasswordValid = this.$.create_password.value.length !== 0
  }

  private onConfirmPasswordInput() {
    this.isConfirmPasswordValid = this.$.confirm_password.value.length !== 0
  }

  private onChange(e: { checked: boolean }) {
    this.isChecked = e.checked
  }

  private canCreateAccount() {
    return this.isEmailAddressValid && this.isAccountNameValid && this.isCreatePasswordValid && this.isConfirmPasswordValid && this.isChecked
  }

  private show() {
    // event.preventDefault()
    // const icon = this.$.password.querySelector('#icon')
    // if (!icon) {
    //   return
    // }
    // const isShowing = icon.getAttribute('name') === 'eye-off'
    // icon.setAttribute('name', isShowing ? 'eye-on' : 'eye-off')
    // this.$.password.setAttribute('type', isShowing ? 'password' : 'text')
  }

  private onForgotPasswordButtonClicked() {
    this.dispatchEvent(new CustomEvent('forgot-password-button-clicked'))
  }

  private isEmailAddressValid: boolean = false
  private isAccountNameValid: boolean = false
  private isCreatePasswordValid: boolean = false
  private isConfirmPasswordValid: boolean = false
  private isChecked: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-create-dialog': SettingsBraveAccountCreateDialogElement;
  }
}

customElements.define(
  SettingsBraveAccountCreateDialogElement.is, SettingsBraveAccountCreateDialogElement)
