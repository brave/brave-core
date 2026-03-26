/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { getCss } from './brave_account_password_input.css.js'
import { getHtml } from './brave_account_password_input.html.js'
import type { PasswordStrengthChangedEventDetail } from './brave_account_password_strength_meter.js'
import type { ToggleVisibilityEventDetail } from './brave_account_password_icons.js'

export type PasswordInputEventDetail = { password: string }

export type PasswordInputConfig =
  | { mode: 'confirmation'; confirmPassword: string }
  | { mode: 'regular' }
  | {
      mode: 'strength'
      onPasswordStrengthChanged: (
        detail: PasswordStrengthChangedEventDetail,
      ) => void
    }

export class BraveAccountPasswordInputElement extends CrLitElement {
  static get is() {
    return 'brave-account-password-input'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      config: { type: Object },
      isCapsLockOn: { type: Boolean },
      isInputFocused: { type: Boolean, state: true },
      label: { type: String },
      password: { type: String, state: true },
      placeholder: { type: String },
    }
  }

  protected focusHandler(detail: { innerEvent: Event }) {
    this.isInputFocused = detail.innerEvent.type === 'focus'
  }

  protected onInput(detail: { value: string }) {
    this.password = detail.value
    this.fire('password-input', {
      password: this.password,
    } satisfies PasswordInputEventDetail)
  }

  protected onToggleVisibility(e: CustomEvent<ToggleVisibilityEventDetail>) {
    ;(e.currentTarget as Element).setAttribute(
      'type',
      e.detail.show ? 'text' : 'password',
    )
  }

  protected onPasswordStrengthChanged(
    e: CustomEvent<PasswordStrengthChangedEventDetail>,
  ) {
    if (this.config.mode === 'strength') {
      this.config.onPasswordStrengthChanged(e.detail)
    }
  }

  protected get confirmPassword() {
    return this.config.mode === 'confirmation'
      ? this.config.confirmPassword
      : ''
  }

  protected get shouldStyleAsError() {
    return (
      this.config.mode === 'confirmation'
      && this.password.length !== 0
      && this.password !== this.confirmPassword
    )
  }

  protected get shouldShowDropdown() {
    return this.config.mode !== 'regular' && this.password.length !== 0
  }

  protected getIconName() {
    if (this.password.length !== 0) {
      this.icon =
        this.password === this.confirmPassword
          ? 'check-circle-filled'
          : 'warning-triangle-filled'
    }

    return this.icon
  }

  protected icon = 'warning-triangle-filled'
  protected accessor config: PasswordInputConfig = { mode: 'regular' }
  protected accessor isCapsLockOn = false
  protected accessor isInputFocused = false
  protected accessor label = ''
  protected accessor password = ''
  protected accessor placeholder = ''
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-password-input': BraveAccountPasswordInputElement
  }
}

customElements.define(
  BraveAccountPasswordInputElement.is,
  BraveAccountPasswordInputElement,
)
