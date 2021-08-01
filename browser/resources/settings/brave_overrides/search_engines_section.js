// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

import '../brave_search_engines_page/brave_search_engines_page.m.js'

RegisterPolymerTemplateModifications({
  'settings-search-page': (templateContent) => {
    const enginesSubpageTrigger = templateContent.getElementById('enginesSubpageTrigger')
    if (!enginesSubpageTrigger) {
      console.error(`[Brave Settings Overrides] Couldn't find enginesSubpageTrigger`)
    } else {
      enginesSubpageTrigger.insertAdjacentHTML('beforebegin', `
        <settings-brave-search-page prefs="{{prefs}}"></settings-brave-search-page>
      `)
    }
  },
})

