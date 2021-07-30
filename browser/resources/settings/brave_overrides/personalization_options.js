// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

import '../brave_privacy_page/brave_personalization_options.m.js'

RegisterPolymerTemplateModifications({
  'settings-personalization-options': (templateContent) => {
    const searchSuggestToggle = templateContent.getElementById('searchSuggestToggle')
    if (!searchSuggestToggle) {
      console.error(`[Brave Settings Overrides] Couldn't find search suggest toggle`)
    } else {
      searchSuggestToggle.insertAdjacentHTML('afterend', `
        <settings-brave-personalization-options prefs="{{prefs}}"></settings-brave-personalization-options>
      `)
    }
  },
})
