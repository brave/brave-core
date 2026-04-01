/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { Error } from './brave_account_common.js'
import { getHtml } from './brave_account_error_dialog.html.js'
import {
  LoginErrorCode,
  RegisterErrorCode,
} from './brave_account.mojom-webui.js'
import { BraveAccountStrings } from './brave_components_webui_strings.js'

const LOGIN_ERROR_STRINGS: Partial<Record<LoginErrorCode, string>> = {
  [LoginErrorCode.kEmailNotVerified]:
    BraveAccountStrings.BRAVE_ACCOUNT_ERROR_DIALOG_EMAIL_NOT_VERIFIED,
  [LoginErrorCode.kIncorrectEmail]:
    BraveAccountStrings.BRAVE_ACCOUNT_ERROR_DIALOG_INCORRECT_EMAIL,
  [LoginErrorCode.kIncorrectPassword]:
    BraveAccountStrings.BRAVE_ACCOUNT_ERROR_DIALOG_INCORRECT_PASSWORD,
}

const REGISTER_ERROR_STRINGS: Partial<Record<RegisterErrorCode, string>> = {
  [RegisterErrorCode.kAccountExists]:
    BraveAccountStrings.BRAVE_ACCOUNT_ERROR_DIALOG_ACCOUNT_EXISTS,
  [RegisterErrorCode.kEmailDomainNotSupported]:
    BraveAccountStrings.BRAVE_ACCOUNT_ERROR_DIALOG_EMAIL_DOMAIN_NOT_SUPPORTED,
  [RegisterErrorCode.kTooManyVerifications]:
    BraveAccountStrings.BRAVE_ACCOUNT_ERROR_DIALOG_TOO_MANY_VERIFICATIONS,
}

function getErrorMessage<T extends LoginErrorCode | RegisterErrorCode>(
  errorStrings: Partial<Record<T, string>>,
  details: { netErrorOrHttpStatus: number | null; errorCode: T | null },
): string {
  const { netErrorOrHttpStatus, errorCode } = details

  const errorLabel = loadTimeData.getString(
    BraveAccountStrings.BRAVE_ACCOUNT_ERROR,
  )

  if (netErrorOrHttpStatus == null) {
    // client-side error
    return loadTimeData.getStringF(
      BraveAccountStrings.BRAVE_ACCOUNT_CLIENT_ERROR,
      errorCode != null ? ` (${errorLabel}=${errorCode})` : '',
    )
  }

  // server-side error
  const specificErrorMessage =
    errorCode != null && errorStrings[errorCode]
      ? loadTimeData.getString(errorStrings[errorCode])
      : null

  return (
    specificErrorMessage
    ?? loadTimeData.getStringF(
      BraveAccountStrings.BRAVE_ACCOUNT_SERVER_ERROR,
      `${netErrorOrHttpStatus > 0 ? 'HTTP' : 'NET'}=${netErrorOrHttpStatus}`,
      errorCode != null ? `, ${errorLabel}=${errorCode}` : '',
    )
  )
}

export class BraveAccountErrorDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-error-dialog'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      error: { type: Object },
    }
  }

  protected get alertMessage(): string {
    switch (this.error.flow) {
      case 'login':
        return getErrorMessage(LOGIN_ERROR_STRINGS, this.error.details)
      case 'register':
        return getErrorMessage(REGISTER_ERROR_STRINGS, this.error.details)
    }
  }

  protected accessor error!: Error
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-error-dialog': BraveAccountErrorDialogElement
  }
}

customElements.define(
  BraveAccountErrorDialogElement.is,
  BraveAccountErrorDialogElement,
)
