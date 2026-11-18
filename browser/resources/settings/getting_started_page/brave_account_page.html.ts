// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {html} from '//resources/lit/v3_0/lit.rollup.js'
import {loadTimeData} from '//resources/js/load_time_data.js'

import {BraveAccountSettingsStrings} from '../brave_components_webui_strings.js'
import {SettingsBraveAccountPageElement} from './brave_account_page.js'

export function getHtml(this: SettingsBraveAccountPageElement) {
  return html`<settings-subpage
    page-title=${loadTimeData.getString(
      BraveAccountSettingsStrings.BRAVE_ACCOUNT_TITLE,
    )}
    class="multi-card"
  >
    <settings-section>
      <brave-account-row></brave-account-row>
    </settings-section>
  </settings-subpage>`
}
