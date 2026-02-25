/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  CrLitElement,
  PropertyValues,
} from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getCss } from './brave_account_password_strength_meter.css.js'
import { getHtml } from './brave_account_password_strength_meter.html.js'

export type PasswordStrengthChangedEventDetail = { isStrongEnough: boolean }

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
      strength: { type: Number, state: true },
      password: { type: String },
    }
  }

  override async updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties)

    if ((changedProperties as Map<PropertyKey, unknown>).has('password')) {
      const password = this.password
      try {
        const { strength } =
          await this.browserProxy.password_strength_meter.getPasswordStrength(
            password,
          )

        // Ignore stale response if password changed while we were waiting.
        if (this.password !== password) {
          return
        }

        this.strength = strength
      } catch (error) {
        console.error('Failed to calculate password strength:', error)
        this.strength = 0
      }

      this.category =
        this.strength < 60 ? 'Weak' : this.strength < 100 ? 'Medium' : 'Strong'

      this.fire('password-strength-changed', {
        isStrongEnough: this.strength === 100,
      })
    }
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor category: 'Weak' | 'Medium' | 'Strong' = 'Weak'
  protected accessor strength = 0
  private accessor password = ''
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
