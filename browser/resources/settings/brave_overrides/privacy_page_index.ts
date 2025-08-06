/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  RegisterPolymerTemplateModifications,
} from 'chrome://resources/brave/polymer_overriding.js'
import {
  html
} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

RegisterPolymerTemplateModifications({
  'settings-privacy-page-index': (templateContent) => {
    // Hide the privacy guide promo (can't remove it entirely like we used to
    // do, as the view manager expects it to exist)
    const privacyGuidePromoTemplate = templateContent.
      querySelector('template[is=dom-if][if="[[isPrivacyGuideAvailable]]"]')
    if (privacyGuidePromoTemplate) {
      privacyGuidePromoTemplate.setAttribute('display', 'none')
    } else {
      throw new Error('[Settings] Missing privacyGuidePromoTemplate')
    }

    // Move the safety hub to the end of the page
    const safetyHubTemplate = templateContent.querySelector(
      'template[is=dom-if][if="[[showPage_(pageVisibility_.safetyHub)]]"]')
    if (safetyHubTemplate && safetyHubTemplate.parentElement) {
      safetyHubTemplate.parentElement.appendChild(safetyHubTemplate)
    } else {
      throw new Error('[Settings] Missing safetyHubTemplate')
    }
  }
})
