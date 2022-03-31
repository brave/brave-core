// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js'
import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

import '../brave_search_engines_page/brave_search_engines_page.m.js'

RegisterPolymerTemplateModifications({
  'settings-search-page': (templateContent) => {
    const enginesSubpageTrigger = templateContent.getElementById('enginesSubpageTrigger')
    if (!enginesSubpageTrigger) {
      console.error(`[Brave Settings Overrides] Couldn't find enginesSubpageTrigger`)
    } else {
      // For some reason with the conditional, ts_library processes this toggle
      // out from HTML if we put it into brave_search_engines_page.html, so
      // instead adding it this way.
<if expr="enable_extensions">
      enginesSubpageTrigger.insertAdjacentHTML('beforebegin', `
        <settings-toggle-button id="webDiscoveryEnabled"
          class="cr-row"
          pref="{{prefs.brave.web_discovery_enabled}}"
          label="${I18nBehavior.i18n('braveWebDiscoveryLabel')}"
          sub-label="${I18nBehavior.i18n('braveWebDiscoverySubLabel')}"
          learn-more-url="${I18nBehavior.i18n('webDiscoveryLearnMoreURL')}"
        </settings-toggle-button>
      `)
</if>
      enginesSubpageTrigger.insertAdjacentHTML('beforebegin', `
        <settings-brave-search-page prefs="{{prefs}}"></settings-brave-search-page>
      `)
    }
  }
})
