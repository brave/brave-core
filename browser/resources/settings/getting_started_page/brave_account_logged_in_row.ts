/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'
// @ts-expect-error: no type definitions are generated for leo.bundle.js
import { leoShowAlert } from '//resources/brave/leo.bundle.js'

import { BraveAccountSettingsStrings } from '../brave_components_webui_strings.js'
import {
  LoggedInState,
  LoggedInVerificationIntent,
  VerificationIntent,
} from '../brave_account.mojom-webui.js'
import {
  ChangePasswordClientErrorCode,
  ChangePasswordError,
} from '../change_password.mojom-webui.js'
import { BraveAccountRowBaseElement } from './brave_account_row_base.js'
import { getCss } from './brave_account_logged_in_row.css.js'
import { getHtml } from './brave_account_logged_in_row.html.js'

export class BraveAccountLoggedInRowElement extends
    BraveAccountRowBaseElement<LoggedInVerificationIntent, LoggedInState> {
  static get is() {
    return 'brave-account-logged-in-row'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      ...super.properties,
      isChangingPassword: { type: Boolean, state: true },
    }
  }

  protected accessor isChangingPassword = false
  private measure?: (text: string) => number
  private resizeObserver?: ResizeObserver

  protected override makeVerificationIntent(
        intent: LoggedInVerificationIntent): VerificationIntent {
    return { loggedInIntent: intent }
  }

  override disconnectedCallback() {
    super.disconnectedCallback()

    this.cleanUpEmailTruncation()
  }

  override updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties)

    if ((changedProperties as Map<PropertyKey, unknown>).has('state')) {
      this.updateEmailTruncation()
    }
  }

  protected onLogOutButtonClicked() {
    this.browserProxy.authentication.logOut()
  }

  protected async onChangePasswordButtonClicked() {
    if (this.isChangingPassword) return
    this.isChangingPassword = true

    let error: ChangePasswordError | undefined

    try {
      await this.browserProxy.authentication.changePasswordVerifyInit(
        this.state.email)
    } catch (e) {
      if (e && typeof e === 'object') {
        error = e as ChangePasswordError
      } else {
        console.error('Unexpected error:', e)
        error = {
          clientError: { errorCode: ChangePasswordClientErrorCode.kUnexpected },
        }
      }
    }

    if (error) {
      leoShowAlert({
        type: 'error',
        title: this.i18n(
          BraveAccountSettingsStrings
            .SETTINGS_BRAVE_ACCOUNT_CHANGE_PASSWORD_ERROR_TITLE),
        content: this.getErrorMessage({ kind: 'changePassword', details: error }),
      }, 30000)
    } else {
      this.openBraveAccountDialog()
    }

    this.isChangingPassword = false
  }

  private updateEmailTruncation() {
    // The `#email` element only exists in the non-verification row, so skip
    // truncation while a password-change verification is pending.
    if (this.state.verification) {
      this.cleanUpEmailTruncation()
      return
    }

    if (!this.resizeObserver) {
      const emailEl =
          this.shadowRoot?.querySelector<HTMLElement>('#email')
      if (!emailEl) return

      const canvas = document.createElement('canvas')
      const ctx = canvas.getContext('2d')!
      ctx.font = getComputedStyle(emailEl).font
      this.measure = (text: string) => ctx.measureText(text).width

      this.resizeObserver = new ResizeObserver(() => {
        this.truncateEmail(emailEl)
      })
      this.resizeObserver.observe(emailEl)
      this.truncateEmail(emailEl)
    }
  }

  private truncateEmail(emailEl: HTMLElement) {
    if (!this.measure) return

    const email = this.state.email
    if (!email) return

    const availableWidth = emailEl.clientWidth
    if (!availableWidth) return

    if (this.measure(email) <= availableWidth) {
      emailEl.textContent = email
      return
    }

    // Use binary search (O(log n)) to find the maximum number of characters
    // that fit within available width. Characters are split evenly between
    // the start and end of the email with '…' in the middle.
    const chars = Array.from(email)
    const makeCandidate = (kept: number): string => {
      const prefixLen = Math.ceil(kept / 2)
      const suffixLen = Math.floor(kept / 2)
      return chars.slice(0, prefixLen).join('') + '…' +
             chars.slice(-suffixLen).join('')
    }

    let truncatedEmail = ''
    for (let low = 0, high = chars.length; low < high; ) {
      const middle = Math.ceil((low + high) / 2)
      const candidate = makeCandidate(middle)
      if (this.measure(candidate) <= availableWidth) {
        truncatedEmail = candidate
        low = middle
      } else {
        high = middle - 1
      }
    }

    emailEl.textContent = truncatedEmail
  }

  private cleanUpEmailTruncation() {
    this.measure = undefined
    this.resizeObserver?.disconnect()
    this.resizeObserver = undefined
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-logged-in-row': BraveAccountLoggedInRowElement
  }
}

customElements.define(
  BraveAccountLoggedInRowElement.is, BraveAccountLoggedInRowElement)
