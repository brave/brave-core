// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-payments-section': (templateContent) => {
    const manageLink = templateContent.querySelector('#manageLink')
    if (!manageLink) {
      console.error('[Brave Settings Overrides] Could not find manage payments link')
    }
    manageLink.remove()
  }
})
