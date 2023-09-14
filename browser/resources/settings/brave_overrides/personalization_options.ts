// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import '../brave_privacy_page/brave_personalization_options.js'

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'

RegisterPolymerTemplateModifications({
  'settings-personalization-options': (templateContent) => {
    const metricsReportingControl = templateContent.getElementById('metricsReportingControl')
    if (!metricsReportingControl) {
      console.error(`[Brave Settings Overrides] Couldn't find metricsReportingControl`)
    } else {
      metricsReportingControl.insertAdjacentHTML(
        'beforebegin',
        getTrustedHTML`
          <settings-brave-personalization-options prefs="{{prefs}}">
          </settings-brave-personalization-options>
        `)
      // Removed because we need to locate metrics reportng option between ours at our settings-brave-personalization-options
      metricsReportingControl.remove()
    }

    // searchSugestToggle is moved to search engines section.
    const searchSuggestToggleTemplate = templateContent.querySelector('template[is="dom-if"][if="[[showSearchSuggestToggle_()]]"]')
    if (!searchSuggestToggleTemplate) {
      console.error('[Brave Settings Overrides] Could not find searchSuggestToggle template')
    } else {
      const searchSuggestToggle = searchSuggestToggleTemplate.content.getElementById('searchSuggestToggle')
      if (!searchSuggestToggle) {
        console.error('[Brave Settings Overrides] Could not find searchSuggestToggle id')
      } else {
        searchSuggestToggle.setAttribute('hidden', 'true')
      }
    }
  },
})
