/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assertNotReachedCase } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import {
  showError,
  showResendVerificationEmailResult,
  toFlowError,
} from './brave_account_shared.js'
import { getHtml } from './brave_account_otp_dialog.html.js'
import {
  LoggedInVerificationIntent,
  LoggedOutVerificationIntent,
  VerificationIntent,
  VerificationIntentFieldTags,
  whichVerificationIntent,
} from './brave_account.mojom-webui.js'
import {
  ChangePasswordClientErrorCode,
  ChangePasswordError,
} from './change_password.mojom-webui.js'
import {
  RegisterClientErrorCode,
  RegisterError,
} from './register.mojom-webui.js'
import {
  ResendVerificationEmailClientErrorCode,
  ResendVerificationEmailError,
} from './resend_verification_email.mojom-webui.js'
import {
  ResetPasswordClientErrorCode,
  ResetPasswordError,
} from './reset_password.mojom-webui.js'

export class BraveAccountOtpDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-otp-dialog'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      intent: { type: Object },
      code: { type: String },
      isCodeValid: { type: Boolean },
      isConfirmingCode: { type: Boolean, state: true },
      isResendingConfirmationEmail: { type: Boolean, state: true },
    }
  }

  protected async onConfirmCodeButtonClicked() {
    if (this.isConfirmingCode) return
    this.isConfirmingCode = true

    switch (whichVerificationIntent(this.intent)) {
      case VerificationIntentFieldTags.LOGGED_OUT_INTENT: {
        const loggedOutIntent = this.intent.loggedOutIntent!
        switch (loggedOutIntent) {
          case LoggedOutVerificationIntent.kRegistration:
            await this.confirmRegistrationCode()
            break
          case LoggedOutVerificationIntent.kResetPassword:
            await this.confirmResetPasswordCode()
            break
          default:
            assertNotReachedCase(loggedOutIntent)
        }
        break
      }
      case VerificationIntentFieldTags.LOGGED_IN_INTENT: {
        const loggedInIntent = this.intent.loggedInIntent!
        switch (loggedInIntent) {
          case LoggedInVerificationIntent.kChangePassword:
            await this.confirmChangePasswordCode()
            break
          default:
            assertNotReachedCase(loggedInIntent)
        }
        break
      }
    }

    this.isConfirmingCode = false
  }

  private async confirmRegistrationCode() {
    try {
      await this.browserProxy.authentication.registerStep3(this.code)
    } catch (e) {
      showError({
        kind: 'register',
        details: toFlowError<RegisterError, RegisterClientErrorCode>(
          e,
          RegisterClientErrorCode.kUnexpected,
        ),
      })
    }
  }

  private async confirmResetPasswordCode() {
    try {
      await this.browserProxy.authentication.resetPasswordStep2(this.code)
    } catch (e) {
      showError({
        kind: 'resetPassword',
        details: toFlowError<ResetPasswordError, ResetPasswordClientErrorCode>(
          e,
          ResetPasswordClientErrorCode.kUnexpected,
        ),
      })
    }
  }

  private async confirmChangePasswordCode() {
    try {
      await this.browserProxy.authentication.changePasswordStep2(this.code)
    } catch (e) {
      showError({
        kind: 'changePassword',
        details: toFlowError<
          ChangePasswordError,
          ChangePasswordClientErrorCode
        >(e, ChangePasswordClientErrorCode.kUnexpected),
      })
    }
  }

  protected async onResendEmailCodeButtonClicked() {
    if (this.isResendingConfirmationEmail) return
    this.isResendingConfirmationEmail = true

    let error: ResendVerificationEmailError | undefined

    try {
      await this.browserProxy.authentication.resendVerificationEmail(
        this.intent,
      )
    } catch (e) {
      error = toFlowError<
        ResendVerificationEmailError,
        ResendVerificationEmailClientErrorCode
      >(e, ResendVerificationEmailClientErrorCode.kUnexpected)
    }

    // brave://account keeps the toast up until it is dismissed.
    showResendVerificationEmailResult(error, 0)

    this.isResendingConfirmationEmail = false
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor intent!: VerificationIntent
  protected accessor code = ''
  protected accessor isCodeValid = false
  protected accessor isConfirmingCode = false
  protected accessor isResendingConfirmationEmail = false
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-otp-dialog': BraveAccountOtpDialogElement
  }
}

customElements.define(
  BraveAccountOtpDialogElement.is,
  BraveAccountOtpDialogElement,
)
