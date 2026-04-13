/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// @ts-expect-error
import { leoShowAlert } from '//resources/brave/leo.bundle.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import {
  LoginError,
  LoginErrorCode,
  RegisterError,
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

function getErrorMessageImpl<T extends LoginErrorCode | RegisterErrorCode>(
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

export type Error =
  | { kind: 'login'; details: LoginError }
  | { kind: 'register'; details: RegisterError }

export function getErrorMessage(error: Error): string {
  switch (error.kind) {
    case 'login':
      return getErrorMessageImpl(LOGIN_ERROR_STRINGS, error.details)
    case 'register':
      return getErrorMessageImpl(REGISTER_ERROR_STRINGS, error.details)
  }
}

export function showError(error: Error) {
  leoShowAlert(
    {
      type: 'error',
      title: loadTimeData.getString(
        BraveAccountStrings.BRAVE_ACCOUNT_ERROR_TOAST_TITLE,
      ),
      content: getErrorMessage(error),
    },
    0,
  )
}
