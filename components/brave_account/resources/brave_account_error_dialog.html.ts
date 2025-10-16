/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js'

import { html } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { BraveAccountErrorDialogElement } from './brave_account_error_dialog.js'
import { Flow, FlowToErrorCode } from './brave_account_common.js'
import { RegisterErrorCode } from './brave_account.mojom-webui.js'

export function getHtml(this: BraveAccountErrorDialogElement) {
  return html`<!--_html_template_start_-->
    <brave-account-dialog
      alert-message=${(() => {
        const REGISTER_ERROR_STRINGS = {
          [RegisterErrorCode.kAccountExists]:
            '$i18n{braveAccountErrorDialogAccountExists}',
          [RegisterErrorCode.kEmailDomainNotSupported]:
            '$i18n{braveAccountErrorDialogEmailDomainNotSupported}',
          [RegisterErrorCode.kTooManyVerifications]:
            '$i18n{braveAccountErrorDialogTooManyVerifications}',
        } satisfies Partial<Record<RegisterErrorCode, string>>

        const ERROR_STRINGS: {
          [F in Flow]: Partial<Record<FlowToErrorCode[F], string>>
        } = {
          register: REGISTER_ERROR_STRINGS,
        }

        const { statusCode, errorCode } = this.error.details

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
          (errorCode != null ? ERROR_STRINGS[this.error.flow][errorCode] : null)
          ?? loadTimeData.getStringF(
            'braveAccountErrorDialogServerError',
            statusCode,
            errorCode != null
              ? `, $i18n{braveAccountErrorDialogError}=${errorCode}`
              : '',
          )
        )
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
