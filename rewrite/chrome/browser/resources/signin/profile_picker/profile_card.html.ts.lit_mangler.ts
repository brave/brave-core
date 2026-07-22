// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle(fragment => {
  const button = fragment.querySelector('#profileCardButton')
  if (!button) {
    throw new Error('profileCardButton not found')
  }
  button.classList.add('plain')
}, t => t.text.includes('id="profileCardButton"'))
