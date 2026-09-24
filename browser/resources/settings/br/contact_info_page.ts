// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { RegisterPolymerTemplateModifications } from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-contact-info-page': (templateContent) => {
    // Hide verified email card.
    const verifiedEmailCard =
      templateContent.querySelector(
          'template[is=dom-if][if="[[isEmailVerificationProtocolEnabled_]]"]')
        ?.closest('.card')
    if (!verifiedEmailCard) {
      throw new Error('[Settings] Unable to find the verified email ' +
        'section on settings-contact-info-page')
    }
    verifiedEmailCard.hidden = true
  }
})
