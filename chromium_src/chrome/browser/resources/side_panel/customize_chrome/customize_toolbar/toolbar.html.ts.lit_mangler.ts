// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Remove #miniToolbarBackground
mangle(
  (element: DocumentFragment) => {
    // Remove the mini toolbar background.
    const miniToolbarBackground = element.querySelector(
      '#miniToolbarBackground',
    )
    if (!miniToolbarBackground) {
        throw new Error(
            '[Customize Chrome > Toolbar] #miniToolbarBackground is gone.',
        )
    }
    
    miniToolbarBackground.remove()
  },
  (template) => template.text.includes('id="miniToolbarBackground"'),
)
