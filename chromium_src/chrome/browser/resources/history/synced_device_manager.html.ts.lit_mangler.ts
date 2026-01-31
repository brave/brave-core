// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle(
  (element) => {
    const signInGuide = element.querySelector('#sign-in-guide')
    if (!signInGuide) {
      throw new Error(
        '[Brave History] Could not find #sign-in-guide. Has upstream changed?',
      )
    }
    signInGuide.remove()
  },
  (literal) => literal.text.includes('id="sign-in-guide"'),
)
