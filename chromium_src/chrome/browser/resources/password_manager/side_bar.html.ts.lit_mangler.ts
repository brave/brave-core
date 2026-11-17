// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Hide the Checkup menu item.
mangle((root) => {
  const checkup = root.querySelector('#checkup')
  if (!checkup) {
    throw new Error(
      `[Brave Password Manager Overrides] Could not find '#checkup'`,
    )
  }
  checkup.remove()
})
