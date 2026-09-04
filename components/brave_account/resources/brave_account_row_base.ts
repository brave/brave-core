/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'
import type { SanitizeInnerHtmlOpts } from '//resources/js/parse_html_subset.js'
import {
  parseHtmlSubset,
  sanitizeInnerHtml,
} from '//resources/js/parse_html_subset.js'
// @ts-expect-error: no type definitions are generated for leo.bundle.js
import { leoShowAlert } from '//resources/brave/leo.bundle.js'

import { BraveAccountRowBrowserProxy } from './brave_account_row_browser_proxy.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'
import { VerificationIntent } from './brave_account.mojom-webui.js'
import {
  ChangePasswordClientErrorCode,
  ChangePasswordError,
  ChangePasswordServerErrorCode,
} from './change_password.mojom-webui.js'
import {
  ResendVerificationEmailClientErrorCode,
  ResendVerificationEmailError,
  ResendVerificationEmailServerErrorCode,
} from './resend_verification_email.mojom-webui.js'

type Error =
  | { kind: 'changePassword'; details: ChangePasswordError }
  | { kind: 'resendVerificationEmail'; details: ResendVerificationEmailError }

const CHANGE_PASSWORD_CLIENT_ERROR_STRINGS: Partial<
  Record<ChangePasswordClientErrorCode, string>
> = {}

const CHANGE_PASSWORD_SERVER_ERROR_STRINGS: Partial<
  Record<ChangePasswordServerErrorCode, string>
> = {
  [ChangePasswordServerErrorCode.kTooManyVerifications]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_REGISTER_TOO_MANY_VERIFICATIONS,
  [ChangePasswordServerErrorCode.kDailyVerificationLimitReachedForEmail]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_DAILY_VERIFICATION_LIMIT_REACHED_FOR_EMAIL,
  [ChangePasswordServerErrorCode.kEmailAlreadyVerified]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_PASSWORD_RESET_EMAIL_ALREADY_VERIFIED,
  [ChangePasswordServerErrorCode.kMaximumCodeVerificationAttemptsExceeded]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_CODE_VERIFICATION_ATTEMPTS_EXCEEDED,
  [ChangePasswordServerErrorCode.kInvalidVerificationCode]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_REGISTER_INVALID_VERIFICATION_CODE,
  [ChangePasswordServerErrorCode.kTokenHasExpired]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_TOKEN_HAS_EXPIRED,
}

const RESEND_VERIFICATION_EMAIL_CLIENT_ERROR_STRINGS: Partial<
  Record<ResendVerificationEmailClientErrorCode, string>
> = {}

const RESEND_VERIFICATION_EMAIL_SERVER_ERROR_STRINGS: Partial<
  Record<ResendVerificationEmailServerErrorCode, string>
> = {
  [ResendVerificationEmailServerErrorCode.kMaximumEmailSendAttemptsExceeded]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_SEND_ATTEMPTS_EXCEEDED,
  [ResendVerificationEmailServerErrorCode.kEmailAlreadyVerified]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ALREADY_VERIFIED,
  [ResendVerificationEmailServerErrorCode.kMaximumCodeVerificationAttemptsExceeded]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_CODE_VERIFICATION_ATTEMPTS_EXCEEDED,
  [ResendVerificationEmailServerErrorCode.kTokenHasExpired]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_TOKEN_HAS_EXPIRED,
}

// Shared by the logged-out and logged-in rows, which differ only in their
// verification intent type (`Intent`) and how it is tagged into a
// `VerificationIntent` (logged-out vs logged-in). `Intent` is the bare
// per-state intent enum carried by `state.verification`.
export abstract class BraveAccountRowBaseElement<
  Intent,
  State extends { verification: { intent: Intent } | null },
> extends CrLitElement {
  // cr_elements only builds i18n_mixin_lit.ts when !is_ios, but these rows are
  // also served as a page there, so I18nMixinLit is not available. These are
  // the two methods of it the rows use, over loadTimeData directly.
  private i18nRaw(id: string, ...varArgs: Array<string | number>) {
    return varArgs.length === 0
      ? loadTimeData.getString(id)
      : loadTimeData.getStringF(id, ...varArgs)
  }

  protected i18n(id: string, ...varArgs: Array<string | number>) {
    return parseHtmlSubset(
      `<b>${this.i18nRaw(id, ...varArgs)}</b>`).firstChild!.textContent!
  }

  protected i18nAdvanced(id: string, opts?: SanitizeInnerHtmlOpts) {
    opts = opts || {}
    return sanitizeInnerHtml(
      this.i18nRaw(id, ...(opts.substitutions || [])), opts)
  }

  static override get properties() {
    return {
      browserProxy: { type: Object },
      initiatingServiceName: { type: String },
      state: { type: Object },
    }
  }

  accessor browserProxy!: BraveAccountRowBrowserProxy
  protected accessor initiatingServiceName = ''
  // `& object` is only here to satisfy the @webui-eslint/lit-property-accessor
  // lint rule, which expects Object reactive properties to be typed as objects.
  // The actual shape of `state` is defined by the `State` constraint above.
  protected accessor state!: State & object

  private isResendingConfirmationEmail = false

  // Tags the bare per-state intent into the union the service expects.
  protected abstract makeVerificationIntent(intent: Intent): VerificationIntent

  protected async onResendConfirmationEmailLinkClicked(
        e: CustomEvent<{event: Event}>) {
    e.detail.event.preventDefault()

    if (this.isResendingConfirmationEmail) return
    this.isResendingConfirmationEmail = true

    let error: ResendVerificationEmailError | undefined

    assert(this.state.verification)
    try {
      await this.browserProxy.authentication.resendVerificationEmail(
        this.makeVerificationIntent(this.state.verification.intent))
    } catch (e) {
      if (e && typeof e === 'object') {
        error = e as ResendVerificationEmailError
      } else {
        console.error('Unexpected error:', e)
        error = {
          clientError: {
            errorCode: ResendVerificationEmailClientErrorCode.kUnexpected,
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
        ? this.getErrorMessage(
              { kind: 'resendVerificationEmail', details: error })
        : this.i18n(
              BraveAccountSettingsStrings
                   .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS)
    }, 30000)

    this.isResendingConfirmationEmail = false
  }

  protected onCancelVerificationButtonClicked() {
    assert(this.state.verification)
    this.browserProxy.authentication.cancelVerification(
      this.makeVerificationIntent(this.state.verification.intent))
  }

  // How authentication is entered is the host's decision, not the row's:
  // desktop opens a ConstrainedWebDialog over brave://settings via RowHandler,
  // which is desktop-only (see //brave/components/brave_account/mojom/BUILD.gn),
  // while mobile navigates to the authentication route in its own tab. So the
  // rows only announce the intent and let their mount act on it.
  protected openBraveAccountDialog() {
    this.fire('open-brave-account-dialog', {
      initiatingServiceName: this.initiatingServiceName,
    })
  }

  protected getErrorMessage(error: Error): string {
    switch (error.kind) {
      case 'changePassword':
        return this.getErrorMessageImpl(
          CHANGE_PASSWORD_CLIENT_ERROR_STRINGS,
          CHANGE_PASSWORD_SERVER_ERROR_STRINGS,
          error.details,
        )
      case 'resendVerificationEmail':
        return this.getErrorMessageImpl(
          RESEND_VERIFICATION_EMAIL_CLIENT_ERROR_STRINGS,
          RESEND_VERIFICATION_EMAIL_SERVER_ERROR_STRINGS,
          error.details,
        )
    }
  }

  private getErrorMessageImpl<
    ClientErrorCode extends
      | ChangePasswordClientErrorCode
      | ResendVerificationEmailClientErrorCode,
    ServerErrorCode extends
      | ChangePasswordServerErrorCode
      | ResendVerificationEmailServerErrorCode,
  >(
    clientErrorStrings: Partial<Record<ClientErrorCode, string>>,
    serverErrorStrings: Partial<Record<ServerErrorCode, string>>,
    error: {
      clientError?: { errorCode: ClientErrorCode } | null
      serverError?: {
        netErrorOrHttpStatus: number
        errorCode: ServerErrorCode
      } | null
    },
  ): string {
    const errorLabel = this.i18n(BraveAccountSettingsStrings.BRAVE_ACCOUNT_ERROR)

    if (error.clientError) {
      const stringId = clientErrorStrings[error.clientError.errorCode]
      if (stringId) {
        return this.i18n(stringId)
      }

      return this.i18n(
          BraveAccountSettingsStrings.BRAVE_ACCOUNT_CLIENT_ERROR,
          ` (${errorLabel}=${error.clientError.errorCode})`,
      )
    }

    const serverError = error.serverError!
    const stringId = serverErrorStrings[serverError.errorCode]
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
