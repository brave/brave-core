/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js'

import { html } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { BraveAccountErrorDialogElement } from './brave_account_error_dialog.js'
import {
  LoginErrorCode,
  RegisterErrorCode,
} from './brave_account.mojom-webui.js'

export function getHtml(this: BraveAccountErrorDialogElement) {
  return html`
    <brave-account-dialog
      alert-message=${(() => {
        const LOGIN_ERROR_STRINGS: Partial<Record<LoginErrorCode, string>> = {
          [LoginErrorCode.kIncorrectEmail]:
            loadTimeData.getString(S.BRAVE_ACCOUNT_ERROR_DIALOG_INCORRECT_EMAIL),
          [LoginErrorCode.kIncorrectPassword]:
            loadTimeData.getString(S.BRAVE_ACCOUNT_ERROR_DIALOG_INCORRECT_PASSWORD),
        }

        const REGISTER_ERROR_STRINGS: Partial<
          Record<RegisterErrorCode, string>
        > = {
          [RegisterErrorCode.kAccountExists]:
            loadTimeData.getString(S.BRAVE_ACCOUNT_ERROR_DIALOG_ACCOUNT_EXISTS),
          [RegisterErrorCode.kEmailDomainNotSupported]:
            loadTimeData.getString(S.BRAVE_ACCOUNT_ERROR_DIALOG_EMAIL_DOMAIN_NOT_SUPPORTED),
          [RegisterErrorCode.kTooManyVerifications]:
            loadTimeData.getString(S.BRAVE_ACCOUNT_ERROR_DIALOG_TOO_MANY_VERIFICATIONS),
        }

        const getErrorMessage = <T extends LoginErrorCode | RegisterErrorCode>(
          errorStrings: Partial<Record<T, string>>,
          details: { statusCode: number | null; errorCode: T | null },
        ): string => {
          const { statusCode, errorCode } = details

          if (statusCode == null) {
            // client-side error
            return loadTimeData.getStringF(
              S.BRAVE_ACCOUNT_CLIENT_ERROR,
              errorCode != null
                ? ` (${loadTimeData.getString(S.BRAVE_ACCOUNT_ERROR)}=${errorCode})`
                : '',
            )
          }

          // server-side error
          return (
            (errorCode != null ? errorStrings[errorCode] : null)
            ?? loadTimeData.getStringF(
              S.BRAVE_ACCOUNT_SERVER_ERROR,
              statusCode,
              errorCode != null
                ? `, ${loadTimeData.getString(S.BRAVE_ACCOUNT_ERROR)}=${errorCode}`
                : '',
            )
          )
        }

        switch (this.error.flow) {
          case 'login':
            return getErrorMessage(LOGIN_ERROR_STRINGS, this.error.details)
          case 'register':
            return getErrorMessage(REGISTER_ERROR_STRINGS, this.error.details)
        }
      })()}
      dialog-description="${loadTimeData.getString(S.BRAVE_ACCOUNT_ERROR_DIALOG_DESCRIPTION)}"
      dialog-title="${loadTimeData.getString(S.BRAVE_ACCOUNT_ERROR_DIALOG_TITLE)}"
    >
      <leo-button
        slot="buttons"
        @click=${() => this.fire('back-button-clicked')}
      >
        ${loadTimeData.getString(S.BRAVE_ACCOUNT_BACK_BUTTON_LABEL)}
      </leo-button>
    </brave-account-dialog>`
}
