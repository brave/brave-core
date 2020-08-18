// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-cookies-page': (templateContent) => {
    const blockThirdPartyIncognitoRadioButton = templateContent.getElementById('blockThirdPartyIncognito')
    if (!blockThirdPartyIncognitoRadioButton) {
      console.error('[Brave Settings Overrides] Could not find blockThirdPartyIncognito id on cookies page.')
    }
    blockThirdPartyIncognitoRadioButton.setAttribute('hidden', 'true')
  }
})
