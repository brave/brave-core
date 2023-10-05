// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {html, RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

import {loadTimeData} from '../i18n_setup.js'

RegisterPolymerTemplateModifications({
  'settings-autofill-section': (templateContent) => {
    const autofillProfileToggle =
        templateContent.getElementById('autofillProfileToggle')
    if (!autofillProfileToggle) {
      console.error(
          `[Brave Settings Overrides] Couldn't find autofillProfileToggle`)
    }
    else {
      autofillProfileToggle.setAttribute(
          'label', `${loadTimeData.getString('autofillEnableProfilesLabel')}`)
      autofillProfileToggle.setAttribute(
          'sub-label',
          `${loadTimeData.getString('autofillEnableProfilesSublabel')}`)
      autofillProfileToggle.parentNode.insertBefore(
          html`
          <settings-toggle-button
            id="autofillAutocompleteToggle"
            no-extension-indicator
            label="${loadTimeData.getString('autofillEnableAutocompleteLabel')}"
            sub-label="${
              loadTimeData.getString('autofillEnableAutocompleteSublabel')}"
            pref="{{prefs.brave.autocomplete}}">
          </settings-toggle-button>
        `,
          autofillProfileToggle.nextSibling)
    }
  }
})
