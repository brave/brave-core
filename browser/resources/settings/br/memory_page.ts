// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  html,
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-memory-page': (templateContent) => {
    // Replace settings-section with its children
    const settingsSection = templateContent.querySelector('settings-section')
    if (!settingsSection) {
      throw new Error(
        '[Settings] Missing settings-section on memory_page')
    }
    
    settingsSection.append(html`<tab-discard-exception-list id="exceptionList" prefs="{{prefs}}">
    </tab-discard-exception-list>`)
  }
})
