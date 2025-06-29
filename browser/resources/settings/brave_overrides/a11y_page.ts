// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-a11y-page': (templateContent) => {

    // Remove the captions-related template
    const enableLiveCaptionTemplate = templateContent.
      querySelector('template[if*="enableLiveCaption_"]')
    if (!enableLiveCaptionTemplate) {
      throw new Error('[Settings] Missing enableLiveCaption_ template')
    }
    enableLiveCaptionTemplate.remove()

    // Remove the captions-related rows
    const captionsRows = templateContent.querySelectorAll('#captions')
    if (!captionsRows) {
      throw new Error('[Settings] Missing captions rows')
    }
    for (const row of captionsRows) {
      row.remove()
    }

    // We hide the image labels toggle button as we don't support this service
    const imageLabelsToggle = templateContent.querySelector(
      'settings-toggle-button[on-change="onA11yImageLabelsChange_"]')
    if (imageLabelsToggle) {
      imageLabelsToggle.setAttribute('hidden', 'true')
    } else {
      throw new Error('[Settings] Missing image labels toggle button')
    }
  }
})
