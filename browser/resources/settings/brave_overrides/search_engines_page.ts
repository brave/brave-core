// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {RegisterStyleOverride} from 'chrome://resources/brave/polymer_overriding.js'
import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

RegisterPolymerTemplateModifications({
    'settings-search-engines-page': (templateContent) => {
        const addSearchEngineButton = templateContent.getElementById("addSearchEngine");
        if(!addSearchEngineButton) {
            console.error("[Brave Settings Overrides] Couldn't find addSearchEngine button");
            return;
        }

        const activeEnginesList = templateContent.getElementById("activeEngines");
        if(!activeEnginesList) {
            console.error("[Brave Settings Overrides] Couldn't find activeEnginesList template");
            return;
        }

        // moving addSearchEngine button below Active Search Engines list
        activeEnginesList.after(addSearchEngineButton);
    }
})

RegisterStyleOverride(
    'settings-search-engines-page',
    html`
      <style>
        #addSearchEngine {
          float: right;
          margin: 0 5% 0 0;
        }
      </style>
    `
 )
