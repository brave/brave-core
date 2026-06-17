// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Change the icon of the add current tab button to the Brave one (without an outline).
mangle(
  (element) => {
    const buttonIcon = element.querySelector('#addCurrentTabButton [slot=prefix-icon]')
    if (!buttonIcon) throw new Error('buttonIcon not found')
    buttonIcon.setAttribute('icon', 'plus-add')
  },
  literal => literal.text.includes('addCurrentTabButton'))
