// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://resources/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-safety-check-passwords-child': (templateContent) => {
    const safetyCheckChild = templateContent.getElementById('safetyCheckChild')
    if (!safetyCheckChild) {
      console.error('[Brave Settings Overrides] Could not find safetyCheckChild id on safety check passwords child page.')
    } else {
      safetyCheckChild.setAttribute('hidden', 'true')
    }
  }
})
