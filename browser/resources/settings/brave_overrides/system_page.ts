// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {html, RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {loadTimeData} from '../i18n_setup.js'
import '../brave_system_page/brave_performance_page.js'

RegisterPolymerTemplateModifications({
  'settings-system-page': (templateContent) => {
    templateContent.appendChild(
      html`
        <settings-toggle-button
          class="cr-row"
          pref="{{prefs.brave.enable_closing_last_tab}}"
          label="${loadTimeData.getString("braveHelpTipsClosingLastTab")}">
        </settings-toggle-button>

        <settings-brave-performance-page prefs="{{prefs}}">
        </settings-brave-performance-page>
      `)
      }
})
