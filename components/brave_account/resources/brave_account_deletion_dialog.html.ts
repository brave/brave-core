/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_dialog.js'
import { BraveAccountDeletionDialogElement } from './brave_account_deletion_dialog.js'

export function getHtml(this: BraveAccountDeletionDialogElement) {
  return html`<!--_html_template_start_-->
    <brave-account-dialog
      dialog-description="$i18n{BRAVE_ACCOUNT_DELETION_DIALOG_DESCRIPTION}"
      dialog-title="$i18n{BRAVE_ACCOUNT_DELETION_DIALOG_TITLE}"
    >
      <div slot="inputs">
        <leo-input
          type="text"
          ?disabled=${this.isSubmitting}
          @input=${this.onInput}
        >
          <div class="label ${this.hasError() ? 'error' : ''}">
            ${this.getConfirmationLabel()}
          </div>
        </leo-input>
      </div>
      <leo-button
        slot="buttons"
        class="delete-account-button"
        ?isDisabled=${!this.isConfirmed() || this.isSubmitting}
        @click=${this.onDeleteAccountButtonClicked}
      >
        $i18n{BRAVE_ACCOUNT_DELETE_ACCOUNT_BUTTON_LABEL}
      </leo-button>
      <leo-button
        slot="buttons"
        kind="plain-faint"
        @click=${() => this.fire('close-dialog')}
      >
        $i18n{BRAVE_ACCOUNT_CANCEL_BUTTON_LABEL}
      </leo-button>
    </brave-account-dialog>
    <!--_html_template_end_-->`
}
