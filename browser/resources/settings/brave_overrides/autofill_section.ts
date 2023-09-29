// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

import {loadTimeData} from '../i18n_setup.js'

RegisterPolymerTemplateModifications({
  'settings-autofill-section': (templateContent) => {
    const autofillProfileToggle =
        templateContent.getElementById('autofillProfileToggle')
    if (!autofillProfileToggle) {
      console.error(
          `[Brave Settings Overrides] Couldn't find autofillProfileToggle`)
      return
    }

    autofillProfileToggle.setAttribute(
        'label', `${loadTimeData.getString('autofillEnableProfilesLabel')}`)
    autofillProfileToggle.setAttribute(
        'sub-label',
        `${loadTimeData.getString('autofillEnableProfilesSublabel')}`)
  }
})
