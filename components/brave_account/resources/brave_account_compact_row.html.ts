/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { BraveAccountCompactRowElement } from './brave_account_compact_row.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'

export function getHtml(this: BraveAccountCompactRowElement) {
  return html`${this.description
    ? html`<div
        class="row"
        @click=${this.navigateToAccountSection}
      >
        <leo-icon name="social-brave-release-favicon-fullheight-color">
        </leo-icon>
        <div class="title-and-description">
          <div class="title">
            ${loadTimeData.getString(
              BraveAccountSettingsStrings.BRAVE_ACCOUNT_TITLE,
            )}
          </div>
          <div
            class="description ${this.actionRequired ? 'action-required' : ''}"
          >
            ${this.displayDescription}
          </div>
        </div>
        <leo-button
          kind="plain-faint"
          size="medium"
          fab
        >
          <leo-icon name="carat-right"></leo-icon>
        </leo-button>
      </div>`
    : nothing}`
}
