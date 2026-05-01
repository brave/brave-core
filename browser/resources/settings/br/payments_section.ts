/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-payments-section': (templateContent) => {
    const manageLink = templateContent.querySelector('#manageLink')
    if (!manageLink) {
      console.error('[Brave Settings Overrides] Could not find manage payments link')
    } else {
      manageLink.setAttribute('hidden', 'true')
    }
    const cardBenefitsFlagEnabledTemplate = templateContent.querySelector(
      'template[is=dom-if][if="[[cardBenefitsFlagEnabled_]]"]')
    if (!cardBenefitsFlagEnabledTemplate) {
      console.error(
        `[Settings] Couldn't find cardBenefitsFlagEnabled_ template`)
    } else {
      cardBenefitsFlagEnabledTemplate.remove()
    }
  }
})
