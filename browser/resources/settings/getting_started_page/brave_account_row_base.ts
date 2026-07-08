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
  ResendConfirmationEmailClientErrorCode,
  ResendConfirmationEmailError,
  ResendConfirmationEmailServerErrorCode,
  VerificationIntent,
} from '../brave_account.mojom-webui.js'
import {
  ChangePasswordClientErrorCode,
  ChangePasswordError,
  ChangePasswordServerErrorCode,
} from '../change_password.mojom-webui.js'

type Error =
  | { kind: 'changePassword'; details: ChangePasswordError }
  | { kind: 'resendConfirmationEmail'; details: ResendConfirmationEmailError }

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
  [ChangePasswordServerErrorCode.kVerificationNotFoundOrInvalidIdOrCode]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_PASSWORD_RESET_VERIFICATION_NOT_FOUND_OR_INVALID_ID_OR_CODE,
  [ChangePasswordServerErrorCode.kEmailAlreadyVerified]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_PASSWORD_RESET_EMAIL_ALREADY_VERIFIED,
  [ChangePasswordServerErrorCode.kMaximumCodeVerificationAttemptsExceeded]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_PASSWORD_RESET_MAXIMUM_CODE_VERIFICATION_ATTEMPTS_EXCEEDED,
  [ChangePasswordServerErrorCode.kInvalidVerificationCode]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_REGISTER_INVALID_VERIFICATION_CODE,
  [ChangePasswordServerErrorCode.kTokenHasExpired]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_TOKEN_HAS_EXPIRED,
}

const RESEND_CONFIRMATION_EMAIL_CLIENT_ERROR_STRINGS: Partial<
  Record<ResendConfirmationEmailClientErrorCode, string>
> = {}

const RESEND_CONFIRMATION_EMAIL_SERVER_ERROR_STRINGS: Partial<
  Record<ResendConfirmationEmailServerErrorCode, string>
> = {
  [ResendConfirmationEmailServerErrorCode.kMaximumEmailSendAttemptsExceeded]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_SEND_ATTEMPTS_EXCEEDED,
  [ResendConfirmationEmailServerErrorCode.kEmailAlreadyVerified]:
    BraveAccountSettingsStrings
      .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ALREADY_VERIFIED,
  [ResendConfirmationEmailServerErrorCode.kTokenHasExpired]:
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
> extends I18nMixinLit(CrLitElement) {
  static override get properties() {
    return {
      browserProxy: { type: Object },
      initiatingServiceName: { type: String },
      state: { type: Object },
    }
  }

  accessor browserProxy!: BraveAccountBrowserProxy
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

    let error: ResendConfirmationEmailError | undefined

    assert(this.state.verification)
    try {
      await this.browserProxy.authentication.resendVerificationEmail(
        this.makeVerificationIntent(this.state.verification.intent))
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
        ? this.getErrorMessage(
              { kind: 'resendConfirmationEmail', details: error })
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

  protected openBraveAccountDialog() {
    this.browserProxy.rowHandler.openDialog(this.initiatingServiceName)
  }

  protected getErrorMessage(error: Error): string {
    switch (error.kind) {
      case 'changePassword':
        return this.getErrorMessageImpl(
          CHANGE_PASSWORD_CLIENT_ERROR_STRINGS,
          CHANGE_PASSWORD_SERVER_ERROR_STRINGS,
          error.details,
        )
      case 'resendConfirmationEmail':
        return this.getErrorMessageImpl(
          RESEND_CONFIRMATION_EMAIL_CLIENT_ERROR_STRINGS,
          RESEND_CONFIRMATION_EMAIL_SERVER_ERROR_STRINGS,
          error.details,
        )
    }
  }

  private getErrorMessageImpl<
    ClientErrorCode extends
      | ChangePasswordClientErrorCode
      | ResendConfirmationEmailClientErrorCode,
    ServerErrorCode extends
      | ChangePasswordServerErrorCode
      | ResendConfirmationEmailServerErrorCode,
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
