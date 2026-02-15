/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { getHtml } from './brave_account_error_dialog.html.js'
import { Error } from './brave_account_common.js'
import { BraveAccountStrings } from './brave_components_webui_strings.js'
import {
  LoginErrorCode,
  RegisterErrorCode,
} from './brave_account.mojom-webui.js'

const LOGIN_ERROR_STRINGS: Partial<Record<LoginErrorCode, string>> = {
  [LoginErrorCode.kIncorrectEmail]:
    '$i18n{BRAVE_ACCOUNT_ERROR_DIALOG_INCORRECT_EMAIL}',
  [LoginErrorCode.kIncorrectPassword]:
    '$i18n{BRAVE_ACCOUNT_ERROR_DIALOG_INCORRECT_PASSWORD}',
}

const REGISTER_ERROR_STRINGS: Partial<Record<RegisterErrorCode, string>> = {
  [RegisterErrorCode.kAccountExists]:
    '$i18n{BRAVE_ACCOUNT_ERROR_DIALOG_ACCOUNT_EXISTS}',
  [RegisterErrorCode.kEmailDomainNotSupported]:
    '$i18n{BRAVE_ACCOUNT_ERROR_DIALOG_EMAIL_DOMAIN_NOT_SUPPORTED}',
  [RegisterErrorCode.kTooManyVerifications]:
    '$i18n{BRAVE_ACCOUNT_ERROR_DIALOG_TOO_MANY_VERIFICATIONS}',
}

const getErrorMessage = <T extends LoginErrorCode | RegisterErrorCode>(
  errorStrings: Partial<Record<T, string>>,
  details: { statusCode: number | null; errorCode: T | null },
): string => {
  const { statusCode, errorCode } = details

  if (statusCode == null) {
    return loadTimeData.getStringF(
      BraveAccountStrings.BRAVE_ACCOUNT_CLIENT_ERROR,
      errorCode != null ? ` ($i18n{BRAVE_ACCOUNT_ERROR}=${errorCode})` : '',
    )
  }

  return (
    (errorCode != null ? errorStrings[errorCode] : null)
    ?? loadTimeData.getStringF(
      BraveAccountStrings.BRAVE_ACCOUNT_SERVER_ERROR,
      statusCode,
      errorCode != null ? `, $i18n{BRAVE_ACCOUNT_ERROR}=${errorCode}` : '',
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

  protected accessor error!: Error

  protected getAlertMessage(): string {
    switch (this.error.flow) {
      case 'login':
        return getErrorMessage(LOGIN_ERROR_STRINGS, this.error.details)
      case 'register':
        return getErrorMessage(REGISTER_ERROR_STRINGS, this.error.details)
    }
  }
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
