// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Don't download favicons from the sites; use the cached favicon instead.
mangle((root) => {
  const downloadedFavicon = root.querySelector('#downloadedFavicon')
  if (!downloadedFavicon) {
    throw new Error(
      `[Brave Password Manager Overrides] Could not find '#downloadedFavicon'`,
    )
  }
  downloadedFavicon.removeAttribute('auto-src')
})
