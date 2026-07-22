// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle(fragment => {
  const anchor = fragment.querySelector('a')
  if (!anchor) {
    throw new Error('anchor not found')
  }
  anchor.href = 'https://support.brave.app/hc/en-us/articles/39077114659597'
})
