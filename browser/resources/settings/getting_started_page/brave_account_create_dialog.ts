/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { getCss } from './brave_account_create_dialog.css.js'
import { getHtml } from './brave_account_create_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-create-dialog'...
 */

export interface SettingsBraveAccountCreateDialogElement {
  $: {
    email_address: HTMLInputElement,
    account_name: HTMLInputElement,
    create_password: HTMLInputElement,
    confirm_password: HTMLInputElement,
    password_strength_indicator: HTMLElement,
    password_strength_value: HTMLElement,
    password_strength_category: HTMLElement,
  }
}

export class SettingsBraveAccountCreateDialogElement extends CrLitElement {
  static get is() {
    return 'settings-brave-account-create-dialog'
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
      isAccountNameValid: { type: Boolean },
      isCreatePasswordValid: { type: Boolean },
      isConfirmPasswordValid: { type: Boolean },
      isChecked: { type: Boolean },
    }
  }

  protected onEmailAddressInput() {
    // https://www.regular-expressions.info
    this.isEmailAddressValid = /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}$/.test(this.$.email_address.value)
  }

  protected onAccountNameInput() {
    this.isAccountNameValid = this.$.account_name.value.length !== 0
  }

  protected onCreatePasswordInput() {
    const regexps = [ /^.{5,}$/, /[a-z]/, /[A-Z]/, /[0-9]/, /[^a-zA-Z0-9]/ ]
    const widths = Array.from({ length: regexps.length + 1 }, (_, i) => `calc(100% * ${i}/${regexps.length})`)
    const cagetories = [ 'error', 'error', 'error', 'warning', 'warning', 'success' ]
    const cagetories2 = [ 'Weak', 'Weak', 'Weak', 'Medium', 'Medium', 'Strong' ]

    const color = (category: string, type: string) => `var(--leo-color-systemfeedback-${category}-${type})`

    const point = regexps.filter(regexp => regexp.test(this.$.create_password.value)).length

    this.$.password_strength_indicator.style.setProperty("--primary-color", color(cagetories[point], 'icon'))
    this.$.password_strength_indicator.style.setProperty("--secondary-color", color(cagetories[point], 'background'))
    this.$.password_strength_indicator.style.setProperty("opacity", point === 0 ? '0' : '1')
    this.$.password_strength_value.style.width = widths[point]
    this.$.password_strength_category.textContent = cagetories2[point]

    this.isCreatePasswordValid = point === regexps.length
  }

  protected onConfirmPasswordInput() {
    this.isConfirmPasswordValid = this.$.confirm_password.value.length !== 0
  }

  protected onChange(e: { checked: boolean }) {
    this.isChecked = e.checked
  }

  protected show() {
    // event.preventDefault()
    // const icon = this.$.password.querySelector('#icon')
    // if (!icon) {
    //   return
    // }
    // const isShowing = icon.getAttribute('name') === 'eye-off'
    // icon.setAttribute('name', isShowing ? 'eye-on' : 'eye-off')
    // this.$.password.setAttribute('type', isShowing ? 'password' : 'text')
  }

  protected isEmailAddressValid: boolean = false
  protected isAccountNameValid: boolean = false
  protected isCreatePasswordValid: boolean = false
  protected isConfirmPasswordValid: boolean = false
  protected isChecked: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-create-dialog': SettingsBraveAccountCreateDialogElement
  }
}

customElements.define(
  SettingsBraveAccountCreateDialogElement.is, SettingsBraveAccountCreateDialogElement)
