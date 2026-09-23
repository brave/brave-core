// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { checkPageForMedia } from '//brave/ios/browser/playlist/resources/playlist_utils.js'

// Scan the document for media once it is ready. This script is re-injected on
// every document recreation, so it re-runs detection for each navigation.
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', () => checkPageForMedia())
} else {
  checkPageForMedia()
}
