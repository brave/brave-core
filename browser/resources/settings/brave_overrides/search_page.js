// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js'
import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

<if expr="enable_extensions">
  RegisterPolymerTemplateModifications({
    'settings-search-page': (templateContent) => {
      const searchExplanation = templateContent.getElementById('searchExplanation')
      searchExplanation.parentNode.insertAdjacentHTML('afterend', `
        <settings-toggle-button
          class="hr"
          pref="{{prefs.brave.web_discovery_enabled}}"
          label="${I18nBehavior.i18n('braveWebDiscoveryLabel')}"
          sub-label="${I18nBehavior.i18n('braveWebDiscoverySubLabel')}"
          learn-more-url="${I18nBehavior.i18n('webDiscoveryLearnMoreURL')}"
        </settings-toggle-button>
      `)
    }
  })
</if>