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
  ResendConfirmationEmailError,
  ResendConfirmationEmailErrorCode,
} from './brave_account.mojom-webui.js'
import { BraveAccountStrings } from './brave_components_webui_strings.js'

export type Error =
  | { kind: 'login'; details: LoginError }
  | { kind: 'register'; details: RegisterError }
  | { kind: 'resendConfirmationEmail'; details: ResendConfirmationEmailError }

const LOGIN_ERROR_STRINGS: Partial<Record<LoginErrorCode, string>> = {
  [LoginErrorCode.kEmailNotVerified]:
    BraveAccountStrings.BRAVE_ACCOUNT_LOGIN_EMAIL_NOT_VERIFIED,
  [LoginErrorCode.kIncorrectEmail]:
    BraveAccountStrings.BRAVE_ACCOUNT_LOGIN_INCORRECT_EMAIL,
  [LoginErrorCode.kIncorrectPassword]:
    BraveAccountStrings.BRAVE_ACCOUNT_LOGIN_INCORRECT_PASSWORD,
}

const REGISTER_ERROR_STRINGS: Partial<Record<RegisterErrorCode, string>> = {
  [RegisterErrorCode.kAccountExists]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_ACCOUNT_EXISTS,
  [RegisterErrorCode.kEmailDomainNotSupported]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_EMAIL_DOMAIN_NOT_SUPPORTED,
  [RegisterErrorCode.kTooManyVerifications]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_TOO_MANY_VERIFICATIONS,
  [RegisterErrorCode.kVerificationNotFoundOrInvalidIdOrCode]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_VERIFICATION_NOT_FOUND_OR_INVALID_ID_OR_CODE,
  [RegisterErrorCode.kMaximumCodeVerificationAttemptsExceeded]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_MAXIMUM_CODE_VERIFICATION_ATTEMPTS_EXCEEDED,
  [RegisterErrorCode.kInvalidVerificationCode]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_INVALID_VERIFICATION_CODE,
}

const RESEND_CONFIRMATION_EMAIL_ERROR_STRINGS: Partial<
  Record<ResendConfirmationEmailErrorCode, string>
> = {
  [ResendConfirmationEmailErrorCode.kMaximumEmailSendAttemptsExceeded]:
    BraveAccountStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_SEND_ATTEMPTS_EXCEEDED,
  [ResendConfirmationEmailErrorCode.kEmailAlreadyVerified]:
    BraveAccountStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ALREADY_VERIFIED,
}

function getErrorMessageImpl<
  T extends
    | LoginErrorCode
    | RegisterErrorCode
    | ResendConfirmationEmailErrorCode,
>(
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

function getErrorMessage(error: Error): string {
  switch (error.kind) {
    case 'login':
      return getErrorMessageImpl(LOGIN_ERROR_STRINGS, error.details)
    case 'register':
      return getErrorMessageImpl(REGISTER_ERROR_STRINGS, error.details)
    case 'resendConfirmationEmail':
      return getErrorMessageImpl(
        RESEND_CONFIRMATION_EMAIL_ERROR_STRINGS,
        error.details,
      )
  }
}

function showAlert(type: 'success' | 'error', title: string, content: string) {
  leoShowAlert({ type, title, content }, 0)
}

export function showSuccess(contentId: string, titleId: string) {
  showAlert(
    'success',
    loadTimeData.getString(titleId),
    loadTimeData.getString(contentId),
  )
}

export function showError(error: Error, titleId?: string) {
  showAlert(
    'error',
    loadTimeData.getString(
      titleId ?? BraveAccountStrings.BRAVE_ACCOUNT_ERROR_TOAST_TITLE,
    ),
    getErrorMessage(error),
  )
}
