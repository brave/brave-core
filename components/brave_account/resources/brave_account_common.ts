/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// @ts-expect-error
import { leoShowAlert } from '//resources/brave/leo.bundle.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import {
  LoginError,
  LoginErrorFieldTags,
  LoginServerErrorCode,
  RegisterError,
  RegisterErrorFieldTags,
  RegisterServerErrorCode,
  ResendConfirmationEmailError,
  ResendConfirmationEmailErrorFieldTags,
  ResendConfirmationEmailServerErrorCode,
  whichLoginError,
  whichRegisterError,
  whichResendConfirmationEmailError,
} from './brave_account.mojom-webui.js'
import { BraveAccountStrings } from './brave_components_webui_strings.js'

export type Error =
  | { kind: 'login'; details: LoginError }
  | { kind: 'register'; details: RegisterError }
  | { kind: 'resendConfirmationEmail'; details: ResendConfirmationEmailError }

const LOGIN_SERVER_ERROR_STRINGS: Partial<
  Record<LoginServerErrorCode, string>
> = {
  [LoginServerErrorCode.kEmailNotVerified]:
    BraveAccountStrings.BRAVE_ACCOUNT_LOGIN_EMAIL_NOT_VERIFIED,
  [LoginServerErrorCode.kIncorrectEmail]:
    BraveAccountStrings.BRAVE_ACCOUNT_LOGIN_INCORRECT_EMAIL,
  [LoginServerErrorCode.kIncorrectPassword]:
    BraveAccountStrings.BRAVE_ACCOUNT_LOGIN_INCORRECT_PASSWORD,
}

const REGISTER_SERVER_ERROR_STRINGS: Partial<
  Record<RegisterServerErrorCode, string>
> = {
  [RegisterServerErrorCode.kAccountExists]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_ACCOUNT_EXISTS,
  [RegisterServerErrorCode.kEmailDomainNotSupported]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_EMAIL_DOMAIN_NOT_SUPPORTED,
  [RegisterServerErrorCode.kTooManyVerifications]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_TOO_MANY_VERIFICATIONS,
  [RegisterServerErrorCode.kVerificationNotFoundOrInvalidIdOrCode]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_VERIFICATION_NOT_FOUND_OR_INVALID_ID_OR_CODE,
  [RegisterServerErrorCode.kMaximumCodeVerificationAttemptsExceeded]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_MAXIMUM_CODE_VERIFICATION_ATTEMPTS_EXCEEDED,
  [RegisterServerErrorCode.kInvalidVerificationCode]:
    BraveAccountStrings.BRAVE_ACCOUNT_REGISTER_INVALID_VERIFICATION_CODE,
}

const RESEND_CONFIRMATION_EMAIL_SERVER_ERROR_STRINGS: Partial<
  Record<ResendConfirmationEmailServerErrorCode, string>
> = {
  [ResendConfirmationEmailServerErrorCode.kMaximumEmailSendAttemptsExceeded]:
    BraveAccountStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_SEND_ATTEMPTS_EXCEEDED,
  [ResendConfirmationEmailServerErrorCode.kEmailAlreadyVerified]:
    BraveAccountStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ALREADY_VERIFIED,
}

function formatClientError(clientErrorCode: number): string {
  const errorLabel = loadTimeData.getString(
    BraveAccountStrings.BRAVE_ACCOUNT_ERROR,
  )
  return loadTimeData.getStringF(
    BraveAccountStrings.BRAVE_ACCOUNT_CLIENT_ERROR,
    ` (${errorLabel}=${clientErrorCode})`,
  )
}

function formatServerError<T extends number>(
  errorStrings: Partial<Record<T, string>>,
  netErrorOrHttpStatus: number,
  serverErrorCode: T,
): string {
  const errorLabel = loadTimeData.getString(
    BraveAccountStrings.BRAVE_ACCOUNT_ERROR,
  )

  const specificErrorMessage = errorStrings[serverErrorCode]
  if (specificErrorMessage) {
    return loadTimeData.getString(specificErrorMessage)
  }

  // `kNull == 0` is the sentinel for "server returned null code".
  return loadTimeData.getStringF(
    BraveAccountStrings.BRAVE_ACCOUNT_SERVER_ERROR,
    `${netErrorOrHttpStatus > 0 ? 'HTTP' : 'NET'}=${netErrorOrHttpStatus}`,
    serverErrorCode !== 0 ? `, ${errorLabel}=${serverErrorCode}` : '',
  )
}

function getErrorMessage(error: Error): string {
  switch (error.kind) {
    case 'login':
      switch (whichLoginError(error.details)) {
        case LoginErrorFieldTags.CLIENT_ERROR:
          return formatClientError(error.details.clientError!.errorCode)
        case LoginErrorFieldTags.SERVER_ERROR:
          return formatServerError(
            LOGIN_SERVER_ERROR_STRINGS,
            error.details.serverError!.netErrorOrHttpStatus,
            error.details.serverError!.errorCode,
          )
      }
    case 'register':
      switch (whichRegisterError(error.details)) {
        case RegisterErrorFieldTags.CLIENT_ERROR:
          return formatClientError(error.details.clientError!.errorCode)
        case RegisterErrorFieldTags.SERVER_ERROR:
          return formatServerError(
            REGISTER_SERVER_ERROR_STRINGS,
            error.details.serverError!.netErrorOrHttpStatus,
            error.details.serverError!.errorCode,
          )
      }
    case 'resendConfirmationEmail':
      switch (whichResendConfirmationEmailError(error.details)) {
        case ResendConfirmationEmailErrorFieldTags.CLIENT_ERROR:
          return formatClientError(error.details.clientError!.errorCode)
        case ResendConfirmationEmailErrorFieldTags.SERVER_ERROR:
          return formatServerError(
            RESEND_CONFIRMATION_EMAIL_SERVER_ERROR_STRINGS,
            error.details.serverError!.netErrorOrHttpStatus,
            error.details.serverError!.errorCode,
          )
      }
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
