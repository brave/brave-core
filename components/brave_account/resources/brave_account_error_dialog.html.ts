/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js'

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountErrorDialogElement } from './brave_account_error_dialog.js'

export function getHtml(this: BraveAccountErrorDialogElement) {
  return html`<!--_html_template_start_-->
    <brave-account-dialog
      alert-message=${this.alertMessage}
      dialog-description="$i18n{BRAVE_ACCOUNT_ERROR_DIALOG_DESCRIPTION}"
      dialog-title="$i18n{BRAVE_ACCOUNT_ERROR_DIALOG_TITLE}"
    >
      <leo-button
        slot="buttons"
        @click=${() => this.fire('back-button-clicked')}
      >
        $i18n{BRAVE_ACCOUNT_BACK_BUTTON_LABEL}
      </leo-button>
    </brave-account-dialog>
    <!--_html_template_end_-->`
}
