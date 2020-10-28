// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'passwords-section': (templateContent) => {
    const checkPasswordsLinkRow = templateContent.querySelector('#checkPasswordsLinkRow')
    if (!checkPasswordsLinkRow) {
      console.error('[Brave Settings Overrides] Could not find checkPasswordsLinkRow in passwords_section')
    } else {
      checkPasswordsLinkRow.remove()
    }
    const manageLink = templateContent.querySelector('#manageLink')
    if (!manageLink) {
      console.error('[Brave Settings Overrides] Could not find manageLink in passwords_section')
    } else {
      manageLink.remove()
    }
  }
})
