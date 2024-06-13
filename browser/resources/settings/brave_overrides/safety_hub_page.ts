/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-safety-hub-page': (templateContent) => {
    const safetyHubPasswordsCard = templateContent.getElementById('passwords')
    if (!safetyHubPasswordsCard) {
      console.error('[Settings] missing SafetyHub passwords card')
    } else {
      safetyHubPasswordsCard.setAttribute('hidden', 'true')
    }
  }
})
