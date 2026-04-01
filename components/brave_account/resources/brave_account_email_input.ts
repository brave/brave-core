/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { getCss } from './brave_account_common.css.js'
import { getHtml } from './brave_account_email_input.html.js'

// Maximum email address length according to this RFC3696 errata:
// https://www.rfc-editor.org/errata/eid1690.
// While maxlength is based on UTF-16 code units (where surrogate pairs count as
// 2 code units), type="email"'s built-in validation filters out non-ASCII
// characters, so maxlength="254" effectively means 254 ASCII characters/254
// bytes. Platform behaviors: desktop and iOS invalidate the value if non-ASCII
// is typed, while Android prevents typing non-ASCII entirely.
export const MAX_EMAIL_LENGTH = 254

export type EmailInputEventDetail = { email: string; isValid: boolean }

export class BraveAccountEmailInputElement extends CrLitElement {
  static get is() {
    return 'brave-account-email-input'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      blockBraveAlias: { type: Boolean },
      email: { type: String, state: true },
      isFormatValid: { type: Boolean, state: true },
    }
  }

  protected onInput(detail: {
    value: string
    innerEvent: Event & { target: HTMLInputElement }
  }) {
    this.email = detail.value.trim()
    this.isFormatValid =
      this.email.length > 0 && detail.innerEvent.target.validity.valid
    this.fire('email-input', {
      email: this.email,
      isValid: this.isValid,
    } satisfies EmailInputEventDetail)
  }

  protected accessor blockBraveAlias = false
  private accessor email = ''
  private accessor isFormatValid = false

  protected get isBraveAlias(): boolean {
    return this.isFormatValid && /@bravealias\.com$/i.test(this.email)
  }

  protected get shouldStyleAsError(): boolean {
    return (
      (this.email.length !== 0 && !this.isFormatValid)
      || (this.blockBraveAlias && this.isBraveAlias)
    )
  }

  private get isValid(): boolean {
    return this.isFormatValid && (!this.blockBraveAlias || !this.isBraveAlias)
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-email-input': BraveAccountEmailInputElement
  }
}

customElements.define(
  BraveAccountEmailInputElement.is,
  BraveAccountEmailInputElement,
)
