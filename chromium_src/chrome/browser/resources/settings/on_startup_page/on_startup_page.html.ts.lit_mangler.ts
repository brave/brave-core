// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// We wanted the nested children (without the header)
mangle((root) => {
  const section = root.querySelector('settings-section')
  if (!section) {
    throw new Error(
      `[Settings] On-startup page: couldn't find settings-section`)
  }
  section.replaceWith(...Array.from(section.childNodes))
})
