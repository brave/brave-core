/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  AsyncDirective,
  CrLitElement,
  directive,
  nothing,
} from '//resources/lit/v3_0/lit.rollup.js'

import { getCss } from './brave_account_common.css.js'
import { getHtml } from './brave_account_password_input.html.js'
import type { PasswordStrengthChangedEventDetail } from './brave_account_password_strength_meter.js'
import type { ToggleVisibilityEventDetail } from './brave_account_password_icons.js'

// maxlength is based on UTF-16 code units (where surrogate pairs count as
// 2 code units). maxlength=8192 allows up to 8192 code units (16kB for BMP,
// up to 32kB with characters outside the BMP, e.g. emojis). This limits
// length in code units, not bytes.
export const MAX_PASSWORD_LENGTH = 8192

// Custom directive that freezes the previously rendered value when `freeze`
// is true. Similar to Lit's `noChange` (not exported by Chromium's Lit wrapper
// from //third_party/lit/v3_0/lit.ts), but instead of preventing the update,
// it reuses the last rendered value.
class FreezeWhenDirective extends AsyncDirective {
  private previousValue: unknown = nothing

  render(freeze: boolean, value: unknown): unknown {
    return freeze ? this.previousValue : (this.previousValue = value)
  }
}

export const freezeWhen = directive(FreezeWhenDirective)

export type PasswordInputEventDetail = { password: string; isValid: boolean }

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
      isValid: this.isValid,
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
      this.password.length !== 0
      && (this.config.mode === 'confirmation'
        ? this.password !== this.confirmPassword
        : this.password !== this.password.trim())
    )
  }

  protected get shouldShowDropdown() {
    return (
      this.password.length !== 0
      && (this.config.mode !== 'regular' || !this.isValid)
    )
  }

  protected get isValid() {
    return this.password.length !== 0 && this.password === this.password.trim()
  }

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
