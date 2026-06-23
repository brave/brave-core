/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountLoggedInRowElement } from './brave_account_logged_in_row.js'
import { BraveAccountSettingsStrings } from '../brave_components_webui_strings.js'

export function getHtml(this: BraveAccountLoggedInRowElement) {
  return html`
    <div class="first-row">
      <div class="circle">
        <leo-icon name="social-brave-release-favicon-fullheight-color">
        </leo-icon>
      </div>
      <div class="title-and-description">
        <div class="title">
          ${this.i18n(BraveAccountSettingsStrings.BRAVE_ACCOUNT_TITLE)}
        </div>
        <div class="description">
          <div id="email">${this.state.email}</div>
        </div>
      </div>
      <leo-button kind="outline"
                  size="small"
                  @click=${this.onLogOutButtonClicked}>
        <leo-icon name="outside" slot="icon-before"></leo-icon>
        ${this.i18n(
            BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_LOG_OUT_BUTTON_LABEL)}
      </leo-button>
    </div>`
}
