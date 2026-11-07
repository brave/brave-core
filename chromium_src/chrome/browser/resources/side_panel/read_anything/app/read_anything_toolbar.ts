// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ReadAnythingToolbarElement } from './read_anything_toolbar-chromium.js'

// Brave ships only a subset of the Read Anything fonts (and adds its own) and
// does not load the upstream fonts stylesheet. Bypass font loading and notify
// completion.
ReadAnythingToolbarElement.prototype.loadFontsStylesheet = function (
  this: ReadAnythingToolbarElement,
) {
  this.setFontsLoaded()
}

export * from './read_anything_toolbar-chromium.js'
