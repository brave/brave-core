/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

// The compact row is only ever reached from brave://settings: mobile always
// renders the detailed presentation.
// <if expr="not is_android and not is_ios">
import './brave_account_compact_row.js'
// </if>
import './brave_account_details.js'
import './brave_account_logged_out_row.js'
import {
  AccountStateFieldTags,
  whichAccountState,
} from './brave_account.mojom-webui.js'
import { BraveAccountRowElement } from './brave_account_row.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'

export function getHtml(this: BraveAccountRowElement) {
  return html`${this.state === undefined
    ? nothing
    : whichAccountState(this.state) === AccountStateFieldTags.LOGGED_IN
      ? this.detailed
        ? html` <brave-account-details
            .browserProxy=${this.browserProxy}
            .initiatingServiceName=${this.initiatingServiceName}
            .state=${this.state.loggedIn}
          >
          </brave-account-details>`
        : html` <brave-account-compact-row
            .description=${this.state.loggedIn?.email}
          >
          </brave-account-compact-row>`
      : this.detailed || !this.state.loggedOut?.verification
        ? html` <brave-account-logged-out-row
            .browserProxy=${this.browserProxy}
            .initiatingServiceName=${this.initiatingServiceName}
            .state=${this.state.loggedOut}
          >
          </brave-account-logged-out-row>`
        : html` <brave-account-compact-row
            .description=${loadTimeData.getString(
              BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_PENDING_VERIFICATION_LABEL,
            )}
          >
          </brave-account-compact-row>`}`
}
