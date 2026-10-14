// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle((root) => {
  // Links to Google Pay. Hidden rather than removed, since the element's `$`
  // declares it.
  const manageLink = root.getElementById('manageLink')
  if (!manageLink) {
    throw new Error(`[Settings] Payments page: couldn't find #manageLink`)
  }
  manageLink.setAttribute('hidden', '')

  const cardBenefitsToggle = root.getElementById('cardBenefitsToggle')
  if (!cardBenefitsToggle) {
    throw new Error(
      `[Settings] Payments page: couldn't find #cardBenefitsToggle`)
  }
  cardBenefitsToggle.setAttribute('hidden', '')
})
