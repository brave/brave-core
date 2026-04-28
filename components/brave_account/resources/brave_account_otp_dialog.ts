/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { BraveAccountStrings } from './brave_components_webui_strings.js'
import { showError, showSuccess } from './brave_account_common.js'
import { getHtml } from './brave_account_otp_dialog.html.js'
import {
  RegisterClientErrorCode,
  RegisterError,
  ResendConfirmationEmailClientErrorCode,
  ResendConfirmationEmailError,
} from './brave_account.mojom-webui.js'

export class BraveAccountOtpDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-otp-dialog'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      code: { type: String },
      isCodeValid: { type: Boolean },
      isResendingConfirmationEmail: { type: Boolean, state: true },
    }
  }

  protected async onConfirmCodeButtonClicked() {
    try {
      await this.browserProxy.authentication.registerVerify(this.code)
    } catch (e) {
      let error: RegisterError

      if (e && typeof e === 'object') {
        error = e as RegisterError
      } else {
        console.error('Unexpected error:', e)
        error = {
          clientError: { errorCode: RegisterClientErrorCode.kUnexpected },
        }
      }

      showError({ kind: 'register', details: error })
    }
  }

  protected async onResendEmailCodeButtonClicked() {
    if (this.isResendingConfirmationEmail) return
    this.isResendingConfirmationEmail = true

    let error: ResendConfirmationEmailError | undefined

    try {
      await this.browserProxy.authentication.resendConfirmationEmail()
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

    if (error) {
      showError(
        { kind: 'resendConfirmationEmail', details: error },
        BraveAccountStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ERROR_TITLE,
      )
    } else {
      showSuccess(
        BraveAccountStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS,
        BraveAccountStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS_TITLE,
      )
    }

    this.isResendingConfirmationEmail = false
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor code = ''
  protected accessor isCodeValid = false
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
