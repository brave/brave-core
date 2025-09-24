/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js'

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { RegisterErrorCode } from './brave_account.mojom-webui.js'
import { Flow, FlowToErrorCode } from './brave_account_common.js'
import { BraveAccountErrorDialogElement } from './brave_account_error_dialog.js'

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
          return 'Some client-side error'
        }

        if (errorCode == null) {
          return `Internal server error (HTTP=${statusCode})`
        }

        return (
          ERROR_STRINGS[this.error.flow][errorCode]
          ?? `Internal server error (HTTP=${statusCode}, error=${errorCode})`
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
