/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { getCss } from './brave_account_password_strength_meter.css.js'
import { getHtml } from './brave_account_password_strength_meter.html.js'

export class BraveAccountPasswordStrengthMeter extends CrLitElement {
  static get is() {
    return 'brave-account-password-strength-meter'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      category: { type: String, reflect: true },
      strength: { type: Number },
    }
  }

  override updated(changedProperties: Map<PropertyKey, unknown>) {
    if (changedProperties.has('strength')) {
      this.category =
        this.strength < 60 ? 'Weak' : this.strength < 100 ? 'Medium' : 'Strong'
    }
  }

  protected accessor category: 'Weak' | 'Medium' | 'Strong' = 'Weak'
  protected accessor strength: number = 0
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-password-strength-meter': BraveAccountPasswordStrengthMeter
  }
}

customElements.define(
  BraveAccountPasswordStrengthMeter.is,
  BraveAccountPasswordStrengthMeter,
)
