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

type FlowError =
  | { kind: 'changePassword'; details: ChangePasswordError }
  | { kind: 'login'; details: LoginError }
  | { kind: 'register'; details: RegisterError }
  | { kind: 'resendVerificationEmail'; details: ResendVerificationEmailError }
  | { kind: 'resetPassword'; details: ResetPasswordError }

// The flow a `FlowError` belongs to. Keying the helpers below off `FlowKind`
// lets a call site name its flow once, instead of also spelling out the error
// type and the code enum that go with it.
type FlowKind = FlowError['kind']

// The error type `kind`'s mojo calls reject with.
type ErrorOf<Kind extends FlowKind> = Extract<
  FlowError,
  { kind: Kind }
>['details']

// A flow's own client error enum. Codes are per-flow: the same number means
// different things across flows (e.g. 2 is `kOpaqueError` for register but
// `kInvalidLoginError` for login), and the same name sits at different numbers
// (`kOpaqueError` is 2 for register, 3 for login). Keying on the flow keeps a
// call site from passing another flow's code.
type ClientErrorCodeOf<Kind extends FlowKind> = NonNullable<
  ErrorOf<Kind>['clientError']
>['errorCode']

// Every flow's client error enum has `kUnexpected`; all but
// `resendVerificationEmail` (which never invokes the OPAQUE bindings) also
// have `kOpaqueError`. Both are read off these enums by name, so a call site
// names neither -- it only passes a code to distinguish one specific OPAQUE
// string from the rest (see `opaqueErrors`).
const CLIENT_ERROR_CODES: {
  [Kind in FlowKind]: {
    kUnexpected: ClientErrorCodeOf<Kind>
    kOpaqueError?: ClientErrorCodeOf<Kind>
  }
} = {
  changePassword: ChangePasswordClientErrorCode,
  login: LoginClientErrorCode,
  register: RegisterClientErrorCode,
  resendVerificationEmail: ResendVerificationEmailClientErrorCode,
  resetPassword: ResetPasswordClientErrorCode,
}

// Mojo rejects with a flow error union, whose active arm is the only key
// present. Anything else reaching a catch block -- an OPAQUE binding's bare
// string, or a TypeError from sending on a closed mojo pipe -- is not a flow
// error, and must not be cast to one.
function isFlowError<Kind extends FlowKind>(e: unknown): e is ErrorOf<Kind> {
  return (
    e !== null
    && typeof e === 'object'
    && ('clientError' in e || 'serverError' in e)
  )
}

// Narrows a caught value to `kind`'s flow error, mapping anything else onto a
// client error. The OPAQUE bindings reject with bare strings: `opaqueErrors`
// maps the ones a caller distinguishes onto their own code, any other string
// becomes the flow's `kOpaqueError`, and everything else -- a TypeError from a
// closed mojo pipe, say -- is genuinely unexpected.
function toFlowError<Kind extends FlowKind>(
  kind: Kind,
  e: unknown,
  opaqueErrors?: Partial<Record<string, ClientErrorCodeOf<Kind>>>,
): ErrorOf<Kind> {
  if (isFlowError<Kind>(e)) {
    return e
  }

  const codes = CLIENT_ERROR_CODES[kind]
  const errorCode =
    typeof e === 'string'
      ? (opaqueErrors?.[e] ?? codes.kOpaqueError)
      : undefined
  if (errorCode !== undefined) {
    return { clientError: { errorCode } } as ErrorOf<Kind>
  }

  console.error('Unexpected error:', e)
  return { clientError: { errorCode: codes.kUnexpected } } as ErrorOf<Kind>
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

// The error enums across all flows. Only ever used as generic *bounds*: a
// value typed as one of these unions could hold another flow's code, so never
// type a parameter or field with them -- see `ClientErrorCodeOf`.
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

// Resolves a flow error to a message, with the string tables passed in. The
// generic bounds tie each table to the same enum as `error`'s codes, so a
// mismatched pair cannot be passed here.
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

// Resolves a flow error to a user-facing message. The switch narrows `error`
// to the flow's own error type in each arm, so the compiler checks that the
// tables and the codes come from the same enums.
function getErrorMessage(error: FlowError): string {
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

// The flows that report their successes, as [title, content]. Membership here
// is exactly what `showSuccess` accepts; every other flow is silent on success.
// `satisfies` keeps the literal's keys, which is what `FlowKindWithOwnSuccess`
// reads back.
const FLOWS_WITH_OWN_SUCCESS = {
  resendVerificationEmail: [
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS_TITLE,
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS,
  ],
} satisfies Partial<Record<FlowKind, [string, string]>>

type FlowKindWithOwnSuccess = keyof typeof FLOWS_WITH_OWN_SUCCESS

// The flows with an error-toast title of their own; every other flow falls back
// to the shared "An error occurred". Annotated rather than `satisfies`, so
// `showError` can look up any flow and read back `undefined`. There is no
// content counterpart: an error's content is computed per error code.
const FLOWS_WITH_OWN_ERROR_TITLE: Partial<Record<FlowKind, string>> = {
  resendVerificationEmail:
    BraveAccountSharedStrings.BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ERROR_TITLE,
}

interface ShowAlertOptions {
  // How long the toast stays up; 0 (the default) keeps it up until dismissed.
  durationMs?: number
}

interface ShowErrorOptions<Kind extends FlowKind> extends ShowAlertOptions {
  // Codes for the specific strings the OPAQUE bindings throw, keyed by the
  // string itself. Any string not listed here becomes the flow's own
  // `kOpaqueError`; a non-string becomes `kUnexpected`.
  opaqueErrors?: Partial<Record<string, ClientErrorCodeOf<Kind>>>
  // An already-translated title, overriding the flow's own, for surfaces with
  // their own (e.g. the brave://settings rows, whose titles live in the
  // `BraveAccountSettings` string group and so must be resolved by the caller,
  // through `I18nMixinLit`).
  title?: string
}

// A success toast's title and content both come from the flow itself, so there
// is no per-flow string to override -- only the inherited `durationMs`. Nothing
// here depends on which flow it is either, which is why `showSuccess` needs no
// type parameter where `showError`, whose `opaqueErrors` values are the flow's
// own enum, does.
interface ShowSuccessOptions extends ShowAlertOptions {}

// Shows the failure of a `kind` request in a toast. `e` is the value a `catch`
// block received: anything that is not a flow error is mapped onto a client
// error, so callers hand over what they caught without narrowing it first.
export function showError<Kind extends FlowKind>(
  kind: Kind,
  e: unknown,
  { opaqueErrors, title, durationMs = 0 }: ShowErrorOptions<Kind> = {},
) {
  leoShowAlert(
    {
      type: 'error',
      title:
        title
        ?? loadTimeData.getString(
          FLOWS_WITH_OWN_ERROR_TITLE[kind]
            ?? BraveAccountSharedStrings.BRAVE_ACCOUNT_ERROR_TOAST_TITLE,
        ),
      content: getErrorMessage({
        kind,
        details: toFlowError(kind, e, opaqueErrors),
      } as FlowError),
    },
    durationMs,
  )
}

// Shows the success of a `kind` request in a toast. Only the flows listed in
// `FLOWS_WITH_OWN_SUCCESS` accept this -- every other flow is silent on
// success, and passing it one is a compile error rather than a call that
// quietly does nothing.
export function showSuccess(
  kind: FlowKindWithOwnSuccess,
  { durationMs = 0 }: ShowSuccessOptions = {},
) {
  const [title, content] = FLOWS_WITH_OWN_SUCCESS[kind]
  leoShowAlert(
    {
      type: 'success',
      title: loadTimeData.getString(title),
      content: loadTimeData.getString(content),
    },
    durationMs,
  )
}
