/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BraveAccountBrowserProxyImpl } from './brave_account_browser_proxy.js';
import { CrLitElement, css, html } from '//resources/lit/v3_0/lit.rollup.js'
import { getCss } from './brave_account_create_dialog.css.js'
import { getHtml } from './brave_account_create_dialog.html.js'
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js';

class PasswordStrengthMeter extends I18nMixinLit(CrLitElement) {
  static get is() {
    return 'password-strength-meter'
  }

  static override get styles() {
    return css`
      :host {
        align-items: center;
        display: flex;
        justify-content: space-between;
      }

      :host([category=Weak]) {
        --primary-color: var(--leo-color-systemfeedback-error-icon);
        --secondary-color: var(--leo-color-systemfeedback-error-background);
      }

      :host([category=Medium]) {
        --primary-color: var(--leo-color-systemfeedback-warning-icon);
        --secondary-color: var(--leo-color-systemfeedback-warning-background);
      }

      :host([category=Strong]) {
        --primary-color: var(--leo-color-systemfeedback-success-icon);
        --secondary-color: var(--leo-color-systemfeedback-success-background);
      }

      .bar {
        background-color: var(--secondary-color);
        border-radius: var(--leo-radius-m);
        height: 4px;
        transition: 750ms;
        width: 376px;
      }

      .strength {
        background-color: var(--primary-color);
        border-radius: var(--leo-radius-m);
        height: 100%;
        transition: 750ms;
        width: calc(1% * var(--strength));
      }

      .text {
        color: var(--primary-color);
        font: var(--leo-font-small-regular);
        transition: 750ms;
      }
    `
  }

  override render() {
    return html`
      <div class="bar">
        <div class="strength" style="--strength: ${this.strength}"></div>
      </div>
      <div class="text">
        ${this.i18n(`braveAccountPasswordStrengthMeter${this.category}`)}
      </div>
    `
  }

  static override get properties() {
    return {
      category: { type: String, reflect: true },
      strength: { type: Number },
      text: { type: String },
    }
  }

  override updated(changedProperties: Map<PropertyKey, unknown>) {
    if (changedProperties.has('strength')) {
      this.category = this.strength < 60
        ? 'Weak'
        : this.strength < 100
          ? 'Medium'
          : 'Strong'
    }
  }

  protected category: string = 'Weak'
  protected strength: number = 0
  protected text: string = ''
}

declare global {
  interface HTMLElementTagNameMap {
    'password-strength-meter': PasswordStrengthMeter
  }
}

customElements.define(
  PasswordStrengthMeter.is, PasswordStrengthMeter)

/**
 * @fileoverview
 * 'settings-brave-account-create-dialog'...
 */

export class SettingsBraveAccountCreateDialogElement extends I18nMixinLit(CrLitElement) {
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
      email: { type: String },
      isAccountNameValid: { type: Boolean },
      isCheckboxChecked: { type: Boolean },
      isEmailBraveAlias: { type: Boolean },
      isEmailValid: { type: Boolean },
      password: { type: String },
      passwordConfirmation: { type: String },
      passwordStrength: { type: Number },
    }
  }

  protected onEmailInput(detail: { value: string }) {
    this.email = detail.value
    this.isEmailValid =
      /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}$/.test(this.email)
    this.isEmailBraveAlias = this.email.endsWith('@bravealias.com')
  }

  protected onAccountNameInput(detail: { value: string }) {
    this.isAccountNameValid = detail.value.length !== 0
  }

  protected onCreatePasswordInput(detail: { value: string }) {
    this.password = detail.value
    this.browserProxy_.getPasswordStrength(this.password).then(
      passwordStrength => this.passwordStrength = passwordStrength
    )
    // this.passwordStrength = this.requirements.filter(
    //   requirement => requirement[1] = requirement[0].test(this.password)
    // ).length / this.requirements.length * 100
  }

  protected onConfirmPasswordInput(detail: { value: string }) {
    this.passwordConfirmation = detail.value
  }

  protected onCheckboxChanged(detail: { checked: boolean }) {
    this.isCheckboxChecked = detail.checked
  }

  protected OnEyeIconClicked(event: Event) {
    event.preventDefault()
    const target = event.target as Element
    const isShowing = target.getAttribute('name') === 'eye-on'
    target.setAttribute('name', isShowing ? 'eye-off' : 'eye-on')
    target.parentElement!.setAttribute('type', isShowing ? 'password' : 'text')
  }

  protected getIconName() {
    if (this.passwordConfirmation.length !== 0) {
      this.icon = this.passwordConfirmation === this.password
        ? 'check-circle-filled'
        : 'warning-triangle-filled'
    }

    return this.icon
  }

  protected email: string = ''
  protected icon: string = 'warning-triangle-filled'
  protected isAccountNameValid: boolean = false
  protected isCheckboxChecked: boolean = false
  protected isEmailBraveAlias: boolean = false
  protected isEmailValid: boolean = false
  protected password: string = ''
  protected passwordConfirmation: string = ''
  protected passwordStrength: number = 0
  protected requirements: Array<[RegExp, boolean, string]> = [
    [
      /[a-z]/,
      false,
      this.i18n('braveAccountPasswordStrengthMeterTooltipLowercaseRequirement')
    ],
    [
      /[A-Z]/,
      false,
      this.i18n('braveAccountPasswordStrengthMeterTooltipUppercaseRequirement')
    ],
    [
      /[0-9]/,
      false,
      this.i18n('braveAccountPasswordStrengthMeterTooltipNumberRequirement')
    ],
    [
      /[^a-zA-Z0-9]/,
      false,
      this.i18n('braveAccountPasswordStrengthMeterTooltipSpecialCharacterRequirement')
    ],
    [
      /^.{5,}$/,
      false,
      this.i18n('braveAccountPasswordStrengthMeterTooltipLengthRequirement')
    ]
  ]
  browserProxy_ = BraveAccountBrowserProxyImpl.getInstance()
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-create-dialog': SettingsBraveAccountCreateDialogElement
  }
}

customElements.define(
  SettingsBraveAccountCreateDialogElement.is, SettingsBraveAccountCreateDialogElement)
