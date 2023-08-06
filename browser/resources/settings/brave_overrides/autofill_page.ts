// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'

import {loadTimeData} from '../i18n_setup.js'

RegisterPolymerTemplateModifications({
  'settings-autofill-page': (templateContent) => {
    const addressesManagerButton =
      templateContent.getElementById('addressesManagerButton')
    if (!addressesManagerButton) {
      console.error(`[Brave Settings Overrides] Couldn't find #addressesManagerButton`)
    } else {
      addressesManagerButton.insertAdjacentHTML(
        'afterend',
        getTrustedHTML`
        <settings-toggle-button
          class="hr"
          id="autofill-private-profies"
          pref="{{prefs.brave.autofill_private_windows}}"
        </settings-toggle-button>
      `)
      const privateProfilesToggle =
        templateContent.getElementById('autofill-private-profies')
      if (!privateProfilesToggle) {
        console.error(
          '[Brave Settings Overrides] Couldn\'t find #privateProfilesToggle toggle')
      } else {
        privateProfilesToggle.setAttribute(
          'label', loadTimeData.getString('autofillInPrivateSettingLabel'))
        privateProfilesToggle.setAttribute(
          'sub-label', loadTimeData.getString('autofillInPrivateSettingDesc'))
      }
    }
  },
})
