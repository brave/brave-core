// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerTemplateModifications,
  RegisterStyleOverride
} from 'chrome://resources/brave/polymer_overriding.js'
import {
  html
} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

RegisterStyleOverride(
  'settings-people-page-index',
  html`
    <style>
      #profile-row
      {
        display: none !important;
      }
    </style>
  `
)

RegisterPolymerTemplateModifications({
  'settings-people-page-index': (templateContent) => {
    // People page needs to think it's in the getStarted section, since it is
    // (we remove the People section as a separate section).
    const page = templateContent.querySelector('settings-animated-pages[section=people]')
//    page.setAttribute('section', 'getStarted')
  },
})
