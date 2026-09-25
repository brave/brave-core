// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Keep the custom image preview in the same local-profile avatar view as presets.
mangle(
  (root) => {
    const avatars = root.querySelector(
      '#selectAvatarWrapper > cr-profile-avatar-selector',
    )
    if (!avatars) {
      throw new Error('Profile customization: could not find the avatar selector')
    }

    avatars.insertAdjacentHTML('beforebegin', `
      \${this.isCustomProfileImageEnabled_() ? html\`
        <br-custom-profile-image-row hide-title>
        </br-custom-profile-image-row>
      \` : ''}
    `)
  },
  (template) => template.text.includes('id="selectAvatarWrapper"'),
)
