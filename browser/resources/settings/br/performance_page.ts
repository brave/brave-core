/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-performance-page': (templateContent) => {
    const discardRingTreatmentToggleButton =
      templateContent.querySelector('#discardRingTreatmentToggleButton')
    if (discardRingTreatmentToggleButton) {
      discardRingTreatmentToggleButton.remove()
    } else {
      console.error('[Settings] Could not find discardRingTreatmentToggleButton')
    }
  }
})
