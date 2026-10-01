// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Use Brave's icon for the "no recommendations" state. Its color is set by
// browser/resources/settings/br/safety_hub_page.ts.
mangle(
  (root) => {
    const emptyStateModule = root.querySelector('#emptyStateModule')
    if (!emptyStateModule) {
      throw new Error(
        `[Settings] Safety Hub page: couldn't find #emptyStateModule`)
    }
    emptyStateModule.setAttribute('header-icon', 'shield-done-filled')
  },
  // Only the nested empty-state template, not the page template around it.
  (t) => t.text.trimStart().startsWith(
    '<settings-safety-hub-module id="emptyStateModule"'),
)
