// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle(fragment => {
  const bookmarksBarIcon = fragment.querySelector(
    'cr-icon.bookmark-icon[slot="folder-icon"][icon="bookmarks:bookmarks-bar"]',
  )
  if (!bookmarksBarIcon) {
    throw new Error('Bookmarks bar icon not found')
  }
  bookmarksBarIcon.remove()
})
