/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_dialog.js'
import { BraveAccountEntryDialogElement } from './brave_account_entry_dialog.js'

export function getHtml(this: BraveAccountEntryDialogElement) {
  return html`<!--_html_template_start_-->
    <brave-account-dialog
      dialog-description="$i18n{BRAVE_ACCOUNT_DESCRIPTION}"
      dialog-title="$i18n{BRAVE_ACCOUNT_ENTRY_DIALOG_TITLE}"
    >
      <leo-button
        slot="buttons"
        @click=${() => this.fire('create-button-clicked')}
      >
        $i18n{BRAVE_ACCOUNT_ENTRY_DIALOG_CREATE_BRAVE_ACCOUNT_BUTTON_LABEL}
      </leo-button>
      <leo-button
        slot="buttons"
        kind="outline"
        @click=${() => this.fire('sign-in-button-clicked')}
      >
        $i18n{BRAVE_ACCOUNT_ALREADY_HAVE_ACCOUNT_SIGN_IN_BUTTON_LABEL}
      </leo-button>
    </brave-account-dialog>
    <!--_html_template_end_-->`
}
