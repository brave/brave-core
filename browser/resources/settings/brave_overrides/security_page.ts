// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-security-page': (templateContent) => {
    const safeBrowsingReportingToggle = templateContent.getElementById('safeBrowsingReportingToggle')
    if (!safeBrowsingReportingToggle) {
      console.error('[Brave Settings Overrides] Could not find safeBrowsingReportingToggle id on security page.')
    } else {
      safeBrowsingReportingToggle.setAttribute('hidden', 'true')
    }
    const safeBrowsingEnhanced = templateContent.getElementById('safeBrowsingEnhanced')
    if (!safeBrowsingEnhanced) {
      console.error('[Brave Settings Overrides] Could not find safeBrowsingEnhanced id on security page.')
    } else {
      safeBrowsingEnhanced.setAttribute('hidden', 'true')
    }
    const passwordsLeakToggle = templateContent.getElementById('passwordsLeakToggle')
    if (!passwordsLeakToggle) {
      console.error('[Brave Settings Overrides] Could not find passwordsLeakToggle id on security page.')
    } else {
      passwordsLeakToggle.setAttribute('hidden', 'true')
    }
    const link = templateContent.getElementById('advanced-protection-program-link')
    if (!link) {
      console.error('[Brave Settings Overrides] Could not find advanced-protection-program-link id on security page.')
    } else {
      link.setAttribute('hidden', 'true')
    }
  }
})
