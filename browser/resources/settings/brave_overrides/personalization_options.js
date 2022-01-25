// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

import '../brave_privacy_page/brave_personalization_options.m.js'

RegisterPolymerTemplateModifications({
  'settings-personalization-options': (templateContent) => {
    const metricsReportingControl = templateContent.getElementById('metricsReportingControl')
    if (!metricsReportingControl) {
      console.error(`[Brave Settings Overrides] Couldn't find metricsReportingControl`)
    } else {
      metricsReportingControl.insertAdjacentHTML('beforebegin', `
        <settings-brave-personalization-options prefs="{{prefs}}"></settings-brave-personalization-options>
      `)
    }
  },
})
