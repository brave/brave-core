/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'
// @ts-expect-error: no type definitions are generated for leo.bundle.js
import { leoShowAlert } from '//resources/brave/leo.bundle.js'

import { BraveAccountBrowserProxy } from './brave_account_browser_proxy.js'
import { BraveAccountSettingsStrings } from '../brave_components_webui_strings.js'
import {
  LoggedOutState,
  ResendConfirmationEmailClientErrorCode,
  ResendConfirmationEmailError,
  ResendConfirmationEmailErrorFieldTags,
  ResendConfirmationEmailServerErrorCode,
  whichResendConfirmationEmailError,
} from '../brave_account.mojom-webui.js'
import { getCss } from './brave_account_logged_out_row.css.js'
import { getHtml } from './brave_account_logged_out_row.html.js'

export class BraveAccountLoggedOutRowElement extends I18nMixinLit(CrLitElement) {
  static get is() {
    return 'brave-account-logged-out-row'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      browserProxy: { type: Object },
      initiatingServiceName: { type: String },
      state: { type: Object },
    }
  }

  accessor browserProxy!: BraveAccountBrowserProxy
  protected accessor initiatingServiceName = ''
  protected accessor state!: LoggedOutState

  private isResendingConfirmationEmail = false

  protected async onResendConfirmationEmailLinkClicked(
        e: CustomEvent<{event: Event}>) {
    e.detail.event.preventDefault()

    if (this.isResendingConfirmationEmail) return
    this.isResendingConfirmationEmail = true

    let error: ResendConfirmationEmailError | undefined

    assert(this.state.verification)
    try {
      await this.browserProxy.authentication.resendVerificationEmail(
        { loggedOutIntent: this.state.verification.intent })
    } catch (e) {
      if (e && typeof e === 'object') {
        error = e as ResendConfirmationEmailError
      } else {
        console.error('Unexpected error:', e)
        error = {
          clientError: {
            errorCode: ResendConfirmationEmailClientErrorCode.kUnexpected,
          },
        }
      }
    }

    leoShowAlert({
      type: error ? 'error' : 'success',
      title: this.i18n(
        error ? BraveAccountSettingsStrings
                     .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ERROR_TITLE :
                BraveAccountSettingsStrings
                     .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS_TITLE),
      content: error
        ? this.getErrorMessage(error)
        : this.i18n(
              BraveAccountSettingsStrings
                   .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS)
    }, 30000)

    this.isResendingConfirmationEmail = false
  }

  protected onCancelVerificationButtonClicked() {
    assert(this.state.verification)
    this.browserProxy.authentication.cancelVerification(
      { loggedOutIntent: this.state.verification.intent })
  }

  protected openBraveAccountDialog() {
    this.browserProxy.rowHandler.openDialog(this.initiatingServiceName)
  }

  private getErrorMessage(error: ResendConfirmationEmailError): string {
    const SERVER_ERROR_STRINGS: Partial<
      Record<ResendConfirmationEmailServerErrorCode, string>
    > = {
      [ResendConfirmationEmailServerErrorCode
          .kMaximumEmailSendAttemptsExceeded]:
        BraveAccountSettingsStrings
             .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_SEND_ATTEMPTS_EXCEEDED,
      [ResendConfirmationEmailServerErrorCode.kEmailAlreadyVerified]:
        BraveAccountSettingsStrings
             .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ALREADY_VERIFIED,
      [ResendConfirmationEmailServerErrorCode.kTokenHasExpired]:
        BraveAccountSettingsStrings
             .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_TOKEN_HAS_EXPIRED,
    }

    const errorLabel = this.i18n(
        BraveAccountSettingsStrings.BRAVE_ACCOUNT_ERROR)

    if (whichResendConfirmationEmailError(error)
            === ResendConfirmationEmailErrorFieldTags.CLIENT_ERROR) {
      return this.i18n(
          BraveAccountSettingsStrings.BRAVE_ACCOUNT_CLIENT_ERROR,
          ` (${errorLabel}=${error.clientError!.errorCode})`,
      )
    }

    const serverError = error.serverError!
    const stringId = SERVER_ERROR_STRINGS[serverError.errorCode]
    if (stringId) {
      return this.i18n(stringId)
    }

    return this.i18n(
        BraveAccountSettingsStrings.BRAVE_ACCOUNT_SERVER_ERROR,
        `${serverError.netErrorOrHttpStatus > 0 ? 'HTTP' : 'NET'}=${
          serverError.netErrorOrHttpStatus
        }`,
        `, ${errorLabel}=${serverError.errorCode}`,
    )
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-logged-out-row': BraveAccountLoggedOutRowElement
  }
}

customElements.define(
  BraveAccountLoggedOutRowElement.is, BraveAccountLoggedOutRowElement)
