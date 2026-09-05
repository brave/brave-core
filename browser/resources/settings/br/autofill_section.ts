// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-autofill-section': (templateContent) => {
    const emailSharedMenu = templateContent.querySelector('#emailSharedMenu')
    if (!emailSharedMenu) {
      console.error('[Settings] Could not find #emailSharedMenu to hide email verification card')
      return
    }
    const emailCard = emailSharedMenu.closest('.card') as HTMLElement | null
    if (!emailCard) {
      console.error('[Settings] Could not find email verification card')
      return
    }
    emailCard.hidden = true
  }
})
