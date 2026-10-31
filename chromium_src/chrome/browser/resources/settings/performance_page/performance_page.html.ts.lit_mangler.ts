// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle((root) => {
  // Upstream's performance_page.ts guards its lookup of this toggle, so it
  // can go.
  const discardRingTreatmentToggle = root.querySelector(
    '#discardRingTreatmentToggleButton')
  if (!discardRingTreatmentToggle) {
    throw new Error(
      `[Settings] Performance page: couldn't find `
      + `#discardRingTreatmentToggleButton`)
  }
  discardRingTreatmentToggle.remove()
})
