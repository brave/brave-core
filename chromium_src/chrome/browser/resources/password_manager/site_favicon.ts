// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { RegisterPolymerTemplateModifications } from '//resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'site-favicon': (templateContent) => {
    const downloadedFavicon =
      templateContent.querySelector('#downloadedFavicon')
    if (!downloadedFavicon) {
      throw new Error(
        `[Brave Password Manager Overrides] Could not find '#downloadedFavicon'`
      )
    } else {
      downloadedFavicon.removeAttribute('auto-src')
    }
  }
})

export * from './site_favicon-chromium.js'
