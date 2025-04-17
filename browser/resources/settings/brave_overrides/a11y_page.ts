// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-a11y-page': (templateContent) => {

    // We remove all the templates related to captions - there are two modes
    // depending on whether `captionSettingsOpensExternally_` is set. We remove
    // both.
    const captionsLinkTemplates = templateContent.
      querySelectorAll('template[if*="captionSettingsOpensExternally_"]')
    if (captionsLinkTemplates.length !== 3) {
      console.error(
        `Expected 3 captionsLinkTemplates, got ${captionsLinkTemplates.length}`
        + ' - this indicates something upstream has changed.'
        + ' The override may need to be updated.')
    }
    for (const link of captionsLinkTemplates) {
      link.remove()
    }

    // We hide the image labels toggle button as we don't support this service
    const imageLabelsToggle = templateContent.querySelector(
      'settings-toggle-button[on-change="onA11yImageLabelsChange_"]')
    if (imageLabelsToggle) {
      imageLabelsToggle.setAttribute('hidden', 'true')
    } else {
      console.error('[Settings] missing image labels toggle button')
    }
  }
})
