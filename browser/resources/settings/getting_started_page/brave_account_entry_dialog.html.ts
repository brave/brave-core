/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js'
import { html } from '//resources/lit/v3_0/lit.rollup.js'
import { SettingsBraveAccountEntryDialogElement } from './brave_account_entry_dialog.js'

export function getHtml(this: SettingsBraveAccountEntryDialogElement) {
  return html`<!--_html_template_start_-->
    <settings-brave-account-dialog text-bottom="$i18n{braveSyncBraveAccountDesc}"
                                   text-top="Get started with your Brave account">
      <div slot="buttons">
        <leo-button @click=${() => this.fire('create-button-clicked')}>
          Create a Brave account
        </leo-button>
        <leo-button kind="outline" @click=${() => this.fire('sign-in-button-clicked')}>
          Already have an account? Sign in
        </leo-button>
      </div>
      <div slot="footer" class="footer">
        <div class="footer-text">
          For advanced users, we also support self-custody of your Brave account keys for an extra layer of privacy. <a href="#">Learn more</a>
        </div>
        <leo-button class="button" kind="plain-faint" @click=${() => this.fire('self-custody-button-clicked')}>
          Use self-custody
        </leo-button>
      </div>
    </settings-brave-account-dialog>
  <!--_html_template_end_-->`
}
