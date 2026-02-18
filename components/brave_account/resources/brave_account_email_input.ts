/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { getCss } from './brave_account_email_input.css.js'
import { getHtml } from './brave_account_email_input.html.js'

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
    this.isFormatValid = detail.innerEvent.target.validity.valid
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
