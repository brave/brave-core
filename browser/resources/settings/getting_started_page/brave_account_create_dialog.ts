/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement, css, html } from '//resources/lit/v3_0/lit.rollup.js'
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl
} from './brave_account_browser_proxy.js'
import { getCss } from './brave_account_create_dialog.css.js'
import { getHtml } from './brave_account_create_dialog.html.js'
import { isEmailValid } from './brave_account_common.js'

// @ts-ignore
import { Registration } from 'chrome://resources/brave/opaque_ke.bundle.js'

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
        width: 100%;
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

  constructor() {
    super();
    this.category = 'Weak';
    this.strength = 0;
  }

  protected declare category: 'Weak' | 'Medium' | 'Strong'
  protected declare strength: number
}

declare global {
  interface HTMLElementTagNameMap {
    'password-strength-meter': PasswordStrengthMeter
  }
}

customElements.define(
  PasswordStrengthMeter.is, PasswordStrengthMeter)

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
      email: { type: String },
      isCheckboxChecked: { type: Boolean },
      isEmailBraveAlias: { type: Boolean },
      isEmailValid: { type: Boolean },
      password: { type: String },
      passwordConfirmation: { type: String },
      passwordStrength: { type: Number },
    }
  }

  protected onEmailInput(detail: { value: string }) {
    this.email = detail.value.trim()
    this.isEmailValid = isEmailValid(this.email)
    this.isEmailBraveAlias = (/@bravealias\.com$/i).test(this.email)
  }

  protected onPasswordInput(detail: { value: string }) {
    this.password = detail.value
    this.browserProxy.handler.getPasswordStrength(this.password).then(
      (value: { strength: number }) =>
        this.passwordStrength = value.strength
    )
  }

  protected onConfirmPasswordInput(detail: { value: string }) {
    this.passwordConfirmation = detail.value
  }

  protected onCheckboxChanged(detail: { checked: boolean }) {
    this.isCheckboxChecked = detail.checked
  }

  // TODO(sszaloki): we should consider exporting `noChange`
  // from third_party/lit/v3_0/lit.ts instead, so that such
  // a workaround is not needed.
  protected getIconName() {
    if (this.passwordConfirmation.length !== 0) {
      this.icon = this.passwordConfirmation === this.password
        ? 'check-circle-filled'
        : 'warning-triangle-filled'
    }

    return this.icon
  }

  constructor() {
    super();
    this.email = '';
    this.icon = 'warning-triangle-filled';
    this.isCheckboxChecked = false;
    this.isEmailBraveAlias = false;
    this.isEmailValid = false;
    this.password = '';
    this.passwordConfirmation = '';
    this.passwordStrength = 0;
  }


  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()
  protected declare email: string
  protected declare icon: string
  protected declare isCheckboxChecked: boolean
  protected declare isEmailBraveAlias: boolean
  protected declare isEmailValid: boolean
  protected declare password: string
  protected declare passwordConfirmation: string
  protected declare passwordStrength: number
  protected registration = new Registration()
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-create-dialog':
    SettingsBraveAccountCreateDialogElement
  }
}

customElements.define(
  SettingsBraveAccountCreateDialogElement.is,
  SettingsBraveAccountCreateDialogElement
)
