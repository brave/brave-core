// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// The Shields exceptions list labels its actions "Shields up" and "Shields
// down" rather than "Allow" and "Block". The bound methods come from
// browser/resources/settings/br/site_list.ts.
mangle(
  (root) => {
    const allow = root.querySelector('#allow')
    const block = root.querySelector('#block')
    if (!allow || !block) {
      throw new Error(`[Settings] site-list: couldn't find #allow or #block`)
    }
    allow.textContent = '${this.getAllowButtonLabel_()}'
    block.textContent = '${this.getBlockButtonLabel_()}'
  },
  (template) => template.text.includes('id="actionMenu"'),
)
