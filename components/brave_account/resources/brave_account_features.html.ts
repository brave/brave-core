/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import './brave_account_link_row.js'
import { BraveAccountFeaturesElement } from './brave_account_features.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'

export function getHtml(this: BraveAccountFeaturesElement) {
  return html`${this.rows.length
    ? html`<div class="title">
          ${loadTimeData.getString(
            BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_FEATURES_TITLE,
          )}
        </div>
        <div class="rows">
          ${this.rows.map(
            (row) =>
              html`<brave-account-link-row
                icon=${row.icon}
                .label=${row.label}
                .page=${row.page}
              >
              </brave-account-link-row>`,
          )}
        </div>`
    : nothing}`
}
