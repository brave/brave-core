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

// https://github.com/brave/leo/blob/main/src/components/checkbox/checkbox.svelte
// https://github.com/brave/leo/blob/main/src/components/input/input.svelte
// https://www.regular-expressions.info

export interface SettingsBraveAccountCreateDialogElement {
  $: {
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
      isPasswordStrong: { type: Boolean },
      isChecked: { type: Boolean },
      password: { type: String },
      passwordConfirmation: { type: String },
    }
  }

  protected onEmailAddressInput(detail: { value: string }) {
    this.isEmailAddressValid = /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}$/.test(detail.value)
  }

  protected onAccountNameInput(detail: { value: string }) {
    this.isAccountNameValid = detail.value.length !== 0
  }

  protected onCreatePasswordInput(detail: { value: string }) {
    this.password = detail.value

    const regexps = [ /^.{5,}$/, /[a-z]/, /[A-Z]/, /[0-9]/, /[^a-zA-Z0-9]/ ]
    const widths = Array.from({ length: regexps.length + 1 }, (_, i) => `calc(100% * ${i}/${regexps.length})`)
    const cagetories = [ 'error', 'error', 'error', 'warning', 'warning', 'success' ]
    const cagetories2 = [ 'Weak', 'Weak', 'Weak', 'Medium', 'Medium', 'Strong' ]

    const color = (category: string, type: string) => `var(--leo-color-systemfeedback-${category}-${type})`

    const point = regexps.filter(regexp => regexp.test(this.password)).length

    this.$.password_strength_indicator.style.setProperty("--primary-color", color(cagetories[point], 'icon'))
    this.$.password_strength_indicator.style.setProperty("--secondary-color", color(cagetories[point], 'background'))
    this.$.password_strength_value.style.width = widths[point]
    this.$.password_strength_category.textContent = cagetories2[point]
    if (point === 0) {
      this.$.password_strength_indicator.classList.remove('visible')
    } else {
      this.$.password_strength_indicator.classList.add('visible')
    }

    this.isPasswordStrong = point === regexps.length
  }

  protected onConfirmPasswordInput(detail: { value: string }) {
    this.passwordConfirmation = detail.value
  }

  protected onChange(detail: { checked: boolean }) {
    this.isChecked = detail.checked
  }

  protected toggle(event: Event) {
    event.preventDefault()
    const target = event.target as Element
    const isShowing = target.getAttribute('name') === 'eye-on'
    target.setAttribute('name', isShowing ? 'eye-off' : 'eye-on')
    target.parentElement!.setAttribute('type', isShowing ? 'password' : 'text')
  }

  protected isEmailAddressValid: boolean = false
  protected isAccountNameValid: boolean = false
  protected isPasswordStrong: boolean = false
  protected isChecked: boolean = false
  protected password: string = ''
  protected passwordConfirmation: string = ''
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-create-dialog': SettingsBraveAccountCreateDialogElement
  }
}

customElements.define(
  SettingsBraveAccountCreateDialogElement.is, SettingsBraveAccountCreateDialogElement)
