// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {html, RegisterPolymerTemplateModifications, RegisterPolymerComponentReplacement} from 'chrome://resources/brave/polymer_overriding.js'
import {BraveSettingsAutofillPageElement} from '../brave_autofill_page/brave_autofill_page.js'
import {loadTimeData} from '../i18n_setup.js'

RegisterPolymerComponentReplacement(
  'settings-autofill-page', BraveSettingsAutofillPageElement
)

RegisterPolymerTemplateModifications({
  'settings-autofill-page': (templateContent) => {
    templateContent.appendChild(html`
        <settings-toggle-button
          class="hr"
          label="${loadTimeData.getString('autofillInPrivateSettingLabel')}"
          sub-label="${loadTimeData.getString('autofillInPrivateSettingDesc')}"
          id="autofill-private-profies"
          pref="{{prefs.brave.autofill_private_windows}}"
          hidden=[[!isAutofillPage_]]
        </settings-toggle-button>
      `)
    }
  }
)
