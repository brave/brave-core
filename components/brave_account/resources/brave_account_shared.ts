/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Error and flow-result handling shared by every Brave Account WebUI:
// brave://account and the Brave Account rows in brave://settings. The strings
// resolved here all live in the `BraveAccountShared` group, which both WebUIs
// register (see brave_account_ui_base.h and
// brave_settings_localized_strings_provider.cc).

// @ts-expect-error: no type definitions are generated for leo.bundle.js
import { leoShowAlert } from '//resources/brave/leo.bundle.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { BraveAccountSharedStrings } from './brave_components_webui_strings.js'
import {
  ChangePasswordClientErrorCode,
  ChangePasswordError,
  ChangePasswordServerErrorCode,
} from './change_password.mojom-webui.js'
import {
  LoginClientErrorCode,
  LoginError,
  LoginServerErrorCode,
} from './login.mojom-webui.js'
import {
  RegisterClientErrorCode,
  RegisterError,
  RegisterServerErrorCode,
} from './register.mojom-webui.js'
import {
  ResendVerificationEmailClientErrorCode,
  ResendVerificationEmailError,
  ResendVerificationEmailServerErrorCode,
} from './resend_verification_email.mojom-webui.js'
import {
  ResetPasswordClientErrorCode,
  ResetPasswordError,
  ResetPasswordServerErrorCode,
} from './reset_password.mojom-webui.js'

export type FlowError =
  | { kind: 'changePassword'; details: ChangePasswordError }
  | { kind: 'login'; details: LoginError }
  | { kind: 'register'; details: RegisterError }
  | { kind: 'resendVerificationEmail'; details: ResendVerificationEmailError }
  | { kind: 'resetPassword'; details: ResetPasswordError }

type ClientErrorCode =
  | ChangePasswordClientErrorCode
  | LoginClientErrorCode
  | RegisterClientErrorCode
  | ResendVerificationEmailClientErrorCode
  | ResetPasswordClientErrorCode

type ServerErrorCode =
  | ChangePasswordServerErrorCode
  | LoginServerErrorCode
  | RegisterServerErrorCode
  | ResendVerificationEmailServerErrorCode
  | ResetPasswordServerErrorCode

// Mojo rejects with a flow error union, whose active arm is the only key
// present. Anything else reaching a catch block (e.g. a TypeError thrown by
// the OPAQUE bindings) is not a flow error, and must not be cast to one.
function isFlowError<Error>(e: unknown): e is Error {
  return (
    !!e && typeof e === 'object' && ('clientError' in e || 'serverError' in e)
  )
}

// Narrows a caught value to a flow error, mapping anything else onto a client
// error: `opaqueErrorCode` for the strings the OPAQUE bindings throw (callers
// that don't invoke them omit it), `unexpectedErrorCode` otherwise.
export function toFlowError<Error, Code extends ClientErrorCode>(
  e: unknown,
  unexpectedErrorCode: Code,
  opaqueErrorCode?: Code,
): Error {
  if (isFlowError<Error>(e)) {
    return e
  }

  if (opaqueErrorCode !== undefined && typeof e === 'string') {
    return { clientError: { errorCode: opaqueErrorCode } } as Error
  }

  console.error('Unexpected error:', e)
  return { clientError: { errorCode: unexpectedErrorCode } } as Error
}

const CHANGE_PASSWORD_CLIENT_ERROR_STRINGS: Partial<
  Record<ChangePasswordClientErrorCode, string>
> = {}

const CHANGE_PASSWORD_SERVER_ERROR_STRINGS: Partial<
  Record<ChangePasswordServerErrorCode, string>
> = {
  [ChangePasswordServerErrorCode.kTooManyVerifications]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_TOO_MANY_VERIFICATIONS,
  [ChangePasswordServerErrorCode.kDailyVerificationLimitReachedForEmail]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_DAILY_VERIFICATION_LIMIT_REACHED_FOR_EMAIL,
  [ChangePasswordServerErrorCode.kEmailAlreadyVerified]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_PASSWORD_RESET_EMAIL_ALREADY_VERIFIED,
  [ChangePasswordServerErrorCode.kMaximumCodeVerificationAttemptsExceeded]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_CODE_VERIFICATION_ATTEMPTS_EXCEEDED,
  [ChangePasswordServerErrorCode.kInvalidVerificationCode]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_INVALID_VERIFICATION_CODE,
  [ChangePasswordServerErrorCode.kTokenHasExpired]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_TOKEN_HAS_EXPIRED,
}

const LOGIN_CLIENT_ERROR_STRINGS: Partial<
  Record<LoginClientErrorCode, string>
> = {
  [LoginClientErrorCode.kInvalidLoginError]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_LOGIN_INCORRECT_PASSWORD,
}

const LOGIN_SERVER_ERROR_STRINGS: Partial<
  Record<LoginServerErrorCode, string>
> = {
  [LoginServerErrorCode.kEmailNotVerified]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_LOGIN_EMAIL_NOT_VERIFIED,
  [LoginServerErrorCode.kEmailDomainNotSupported]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_EMAIL_DOMAIN_NOT_SUPPORTED,
  [LoginServerErrorCode.kIncorrectEmail]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_LOGIN_INCORRECT_EMAIL,
  [LoginServerErrorCode.kIncorrectPassword]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_LOGIN_INCORRECT_PASSWORD,
}

const REGISTER_CLIENT_ERROR_STRINGS: Partial<
  Record<RegisterClientErrorCode, string>
> = {}

const REGISTER_SERVER_ERROR_STRINGS: Partial<
  Record<RegisterServerErrorCode, string>
> = {
  [RegisterServerErrorCode.kAccountExists]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_ACCOUNT_EXISTS,
  [RegisterServerErrorCode.kEmailDomainNotSupported]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_EMAIL_DOMAIN_NOT_SUPPORTED,
  [RegisterServerErrorCode.kTooManyVerifications]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_TOO_MANY_VERIFICATIONS,
  [RegisterServerErrorCode.kMaximumCodeVerificationAttemptsExceeded]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_CODE_VERIFICATION_ATTEMPTS_EXCEEDED,
  [RegisterServerErrorCode.kInvalidVerificationCode]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_INVALID_VERIFICATION_CODE,
  [RegisterServerErrorCode.kRegistrationVerificationAlreadyPendingForThisEmail]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_REGISTRATION_VERIFICATION_ALREADY_PENDING_FOR_THIS_EMAIL,
  [RegisterServerErrorCode.kDailyVerificationLimitReachedForEmail]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_DAILY_VERIFICATION_LIMIT_REACHED_FOR_EMAIL,
  [RegisterServerErrorCode.kTokenHasExpired]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_TOKEN_HAS_EXPIRED,
}

const RESEND_VERIFICATION_EMAIL_CLIENT_ERROR_STRINGS: Partial<
  Record<ResendVerificationEmailClientErrorCode, string>
> = {}

const RESEND_VERIFICATION_EMAIL_SERVER_ERROR_STRINGS: Partial<
  Record<ResendVerificationEmailServerErrorCode, string>
> = {
  [ResendVerificationEmailServerErrorCode.kMaximumEmailSendAttemptsExceeded]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_SEND_ATTEMPTS_EXCEEDED,
  [ResendVerificationEmailServerErrorCode.kEmailAlreadyVerified]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ALREADY_VERIFIED,
  [ResendVerificationEmailServerErrorCode.kMaximumCodeVerificationAttemptsExceeded]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_CODE_VERIFICATION_ATTEMPTS_EXCEEDED,
  [ResendVerificationEmailServerErrorCode.kTokenHasExpired]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_TOKEN_HAS_EXPIRED,
}

const RESET_PASSWORD_CLIENT_ERROR_STRINGS: Partial<
  Record<ResetPasswordClientErrorCode, string>
> = {}

const RESET_PASSWORD_SERVER_ERROR_STRINGS: Partial<
  Record<ResetPasswordServerErrorCode, string>
> = {
  [ResetPasswordServerErrorCode.kTooManyVerifications]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_TOO_MANY_VERIFICATIONS,
  [ResetPasswordServerErrorCode.kAccountDoesNotExist]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_ACCOUNT_DOES_NOT_EXIST,
  [ResetPasswordServerErrorCode.kEmailDomainNotSupported]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_EMAIL_DOMAIN_NOT_SUPPORTED,
  [ResetPasswordServerErrorCode.kDailyVerificationLimitReachedForEmail]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_DAILY_VERIFICATION_LIMIT_REACHED_FOR_EMAIL,
  [ResetPasswordServerErrorCode.kEmailAlreadyVerified]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_PASSWORD_RESET_EMAIL_ALREADY_VERIFIED,
  [ResetPasswordServerErrorCode.kMaximumCodeVerificationAttemptsExceeded]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_CODE_VERIFICATION_ATTEMPTS_EXCEEDED,
  [ResetPasswordServerErrorCode.kInvalidVerificationCode]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_REGISTER_INVALID_VERIFICATION_CODE,
  [ResetPasswordServerErrorCode.kTokenHasExpired]:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_TOKEN_HAS_EXPIRED,
}

function getErrorMessageImpl<
  Client extends ClientErrorCode,
  Server extends ServerErrorCode,
>(
  clientErrorStrings: Partial<Record<Client, string>>,
  serverErrorStrings: Partial<Record<Server, string>>,
  error: {
    clientError?: { errorCode: Client } | null
    serverError?: {
      netErrorOrHttpStatus: number
      errorCode: Server
    } | null
  },
): string {
  const errorLabel = loadTimeData.getString(
    BraveAccountSharedStrings.BRAVE_ACCOUNT_ERROR,
  )

  if (error.clientError) {
    const stringId = clientErrorStrings[error.clientError.errorCode]
    if (stringId) {
      return loadTimeData.getString(stringId)
    }

    return loadTimeData.getStringF(
      BraveAccountSharedStrings.BRAVE_ACCOUNT_CLIENT_ERROR,
      ` (${errorLabel}=${error.clientError.errorCode})`,
    )
  }

  const serverError = error.serverError!
  const stringId = serverErrorStrings[serverError.errorCode]
  if (stringId) {
    return loadTimeData.getString(stringId)
  }

  return loadTimeData.getStringF(
    BraveAccountSharedStrings.BRAVE_ACCOUNT_SERVER_ERROR,
    `${serverError.netErrorOrHttpStatus > 0 ? 'HTTP' : 'NET'}=${
      serverError.netErrorOrHttpStatus
    }`,
    `, ${errorLabel}=${serverError.errorCode}`,
  )
}

export function getErrorMessage(error: FlowError): string {
  switch (error.kind) {
    case 'changePassword':
      return getErrorMessageImpl(
        CHANGE_PASSWORD_CLIENT_ERROR_STRINGS,
        CHANGE_PASSWORD_SERVER_ERROR_STRINGS,
        error.details,
      )
    case 'login':
      return getErrorMessageImpl(
        LOGIN_CLIENT_ERROR_STRINGS,
        LOGIN_SERVER_ERROR_STRINGS,
        error.details,
      )
    case 'register':
      return getErrorMessageImpl(
        REGISTER_CLIENT_ERROR_STRINGS,
        REGISTER_SERVER_ERROR_STRINGS,
        error.details,
      )
    case 'resendVerificationEmail':
      return getErrorMessageImpl(
        RESEND_VERIFICATION_EMAIL_CLIENT_ERROR_STRINGS,
        RESEND_VERIFICATION_EMAIL_SERVER_ERROR_STRINGS,
        error.details,
      )
    case 'resetPassword':
      return getErrorMessageImpl(
        RESET_PASSWORD_CLIENT_ERROR_STRINGS,
        RESET_PASSWORD_SERVER_ERROR_STRINGS,
        error.details,
      )
  }
}

// Shows a flow error in a toast that stays up until it is dismissed.
export function showError(error: FlowError) {
  leoShowAlert(
    {
      type: 'error',
      title: loadTimeData.getString(
        BraveAccountSharedStrings.BRAVE_ACCOUNT_ERROR_TOAST_TITLE,
      ),
      content: getErrorMessage(error),
    },
    0,
  )
}

// Shows the outcome of a "resend verification email" request. `durationMs` is
// per-WebUI: brave://account keeps the toast up until dismissed, the
// brave://settings rows time it out.
export function showResendVerificationEmailResult(
  error: ResendVerificationEmailError | undefined,
  durationMs: number,
) {
  leoShowAlert(
    {
      type: error ? 'error' : 'success',
      title: loadTimeData.getString(
        error
          ? BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ERROR_TITLE
          : BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS_TITLE,
      ),
      content: error
        ? getErrorMessage({ kind: 'resendVerificationEmail', details: error })
        : loadTimeData.getString(
            BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS,
          ),
    },
    durationMs,
  )
}
