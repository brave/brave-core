/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '../brave_privacy_page/brave_personalization_options.js'

import {
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-personalization-options': (templateContent) => {
    const metricsConsentRestructureTemplate = templateContent.querySelector(
      'template[is="dom-if"][if="[[!shouldUseMetricsConsentRestructure_]]"]')
    if (!metricsConsentRestructureTemplate) {
      console.error('[Settings] Could not find metrics consent template')
    } else {
      const metricsReportingControl = metricsConsentRestructureTemplate.
        content.getElementById('metricsReportingControl')
      if (!metricsReportingControl) {
        console.error(`[Settingss] Couldn't find metricsReportingControl`)
      } else {
        // Removed because we need to locate the metrics reporting option
        // between ours at our settings-brave-personalization-options
        metricsReportingControl.remove()
      }
    }

    // searchSugestToggle is moved to search engines section.
    const searchSuggestToggleTemplate = templateContent.querySelector(
      'template[is="dom-if"][if="[[showSearchSuggestToggle_()]]"]')
    if (!searchSuggestToggleTemplate) {
      console.error('[Settings] Could not find searchSuggestToggle template')
    } else {
      const searchSuggestToggle = searchSuggestToggleTemplate.content.
        getElementById('searchSuggestToggle')
      if (!searchSuggestToggle) {
        console.error('[Settings] Could not find searchSuggestToggle id')
      } else {
        searchSuggestToggle.setAttribute('hidden', 'true')
      }
    }
  },
})
