// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import '../brave_search_engines_page/brave_search_engines_page.js'

import '../brave_search_engines_page/private_search_engine_list_dialog.js';

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'
import {loadTimeData} from '../i18n_setup.js'

RegisterPolymerTemplateModifications({
  'settings-search-page': (templateContent) => {
    const enginesSubpageTrigger = templateContent.getElementById('enginesSubpageTrigger')
    if (!enginesSubpageTrigger) {
      console.error(`[Settings] Couldn't find enginesSubpageTrigger`)
    } else {
      enginesSubpageTrigger.insertAdjacentHTML(
        'beforebegin',
        getTrustedHTML`
          <settings-brave-search-page prefs="{{prefs}}">
          </settings-brave-search-page>
        `)
    }
    const searchEngineTitleElement =
      templateContent.querySelector('.default-search-engine')
    if (searchEngineTitleElement?.firstChild?.nodeType === Node.TEXT_NODE) {
      searchEngineTitleElement.firstChild.textContent =
        loadTimeData.getString('normalSearchEnginesSiteSearchEngineHeading')
    } else {
      console.error(`[Settings] Couldn't find search engine title text node`)
    }
  }
})
