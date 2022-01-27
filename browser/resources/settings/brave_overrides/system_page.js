// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'
import '../brave_system_page/brave_system_page.js'  

RegisterPolymerTemplateModifications({
  'settings-system-page': (templateContent) => {
    const hardwareAccelToggle = templateContent.getElementById('hardwareAcceleration')
    if (!hardwareAccelToggle) {
      console.error(`[Brave Settings Overrides] Couldn't find hardwareAcceleration toggle`)
    } else {
      hardwareAccelToggle.insertAdjacentHTML('afterend', `
        <settings-brave-system-page prefs="{{prefs}}"></settings-brave-system-page>
      `)
    } 
  }
})
