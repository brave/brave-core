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
  return html`<!--_html_template_start_-->
    <brave-account-dialog
      alert-message=${(() => {
        const LOGIN_ERROR_STRINGS: Partial<Record<LoginErrorCode, string>> = {
          [LoginErrorCode.kIncorrectEmail]:
            '$i18n{braveAccountErrorDialogIncorrectEmail}',
          [LoginErrorCode.kIncorrectPassword]:
            '$i18n{braveAccountErrorDialogIncorrectPassword}',
        }

        const REGISTER_ERROR_STRINGS: Partial<
          Record<RegisterErrorCode, string>
        > = {
          [RegisterErrorCode.kAccountExists]:
            '$i18n{braveAccountErrorDialogAccountExists}',
          [RegisterErrorCode.kEmailDomainNotSupported]:
            '$i18n{braveAccountErrorDialogEmailDomainNotSupported}',
          [RegisterErrorCode.kTooManyVerifications]:
            '$i18n{braveAccountErrorDialogTooManyVerifications}',
        }

        const getErrorMessage = <T extends LoginErrorCode | RegisterErrorCode>(
          errorStrings: Partial<Record<T, string>>,
          details: { statusCode: number | null; errorCode: T | null },
        ): string => {
          const { statusCode, errorCode } = details

          if (statusCode == null) {
            // client-side error
            return loadTimeData.getStringF(
              'braveAccountErrorDialogClientError',
              errorCode != null
                ? ` ($i18n{braveAccountErrorDialogError}=${errorCode})`
                : '',
            )
          }

          // server-side error
          return (
            (errorCode != null ? errorStrings[errorCode] : null)
            ?? loadTimeData.getStringF(
              'braveAccountErrorDialogServerError',
              statusCode,
              errorCode != null
                ? `, $i18n{braveAccountErrorDialogError}=${errorCode}`
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
      dialog-description="$i18n{braveAccountErrorDialogDescription}"
      dialog-title="$i18n{braveAccountErrorDialogTitle}"
    >
      <leo-button
        slot="buttons"
        @click=${() => this.fire('back-button-clicked')}
      >
        $i18n{braveAccountBackButtonLabel}
      </leo-button>
    </brave-account-dialog>
    <!--_html_template_end_-->`
}
