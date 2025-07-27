// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  html,
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-main': (templateContent) => {
    // Always show settings-basic-page
    const templateSettingsBasicPage =
      templateContent.querySelector('#old template')
    if (templateSettingsBasicPage) {
      templateSettingsBasicPage.setAttribute('if', 'true')
    } else {
      throw new Error('[Settings] Missing template for settings-basic-page')
    }

    // Remove the performance page template
    const templatePerformancePageSlot = templateContent.querySelector(
      'template[is=dom-if][if="[[showPage_(pageVisibility_.performance)]]"]')
    if (templatePerformancePageSlot) {
      templatePerformancePageSlot.remove()
    } else {
      throw new Error('[Settings] Missing template for performance page slot')
    }
  }
})
