// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-a11y-page': (templateContent) => {

    // We remove/hide all elements related to captions.
    // <if expr="is_macosx or is_win">
    const enableLiveCaptionTemplate = templateContent.
      querySelector('template[is=dom-if][if="[[enableLiveCaption_]]"]')
    if (!enableLiveCaptionTemplate) {
      console.error(
        `[Settings][a11y] Couldn't find enableLiveCaption_ template`)
    } else {
      enableLiveCaptionTemplate.remove()
    }
    // </if>
    const captions = templateContent.getElementById('captions')
    if (!captions) {
        console.error(`[Settings][a11y] Couldn't find #captions`)
    } else {
        captions.remove()
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
