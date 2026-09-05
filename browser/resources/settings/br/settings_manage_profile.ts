// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerTemplateModifications,
  RegisterStyleOverride,
} from 'chrome://resources/brave/polymer_overriding.js'
import { html } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

// Brave avatar assets are organized in 7-color groups per style under
// `app/theme/default_100_percent/common/avatars`. Chromium's default 6-column
// picker splits each group across multiple rows. Force the use of 7 columns to
// keep variants visually aligned.
const kManageProfilePickerColumns = '7'

RegisterStyleOverride(
  'settings-manage-profile',
  html`
    <style include="settings-shared">
      .content {
        --cr-section-indent-width: 20px;
      }

      .cr-row.manage-profile-section {
        padding-top: var(--leo-spacing-xl) !important;
      }

      .grid-container {
        --icon-grid-gap: 22px !important;
        --icon-size: 66px !important;
      }
    </style>
  `,
)

RegisterPolymerTemplateModifications({
  'settings-manage-profile': (templateContent) => {
    const themeColorPicker = templateContent.querySelector(
      'cr-theme-color-picker',
    )
    if (!themeColorPicker) {
      throw new Error('[Settings] Missing Manage Profile theme color picker')
    }
    themeColorPicker.setAttribute('columns', kManageProfilePickerColumns)

    const profileAvatarSelector = templateContent.querySelector(
      'cr-profile-avatar-selector',
    )
    if (!profileAvatarSelector) {
      throw new Error('[Settings] Missing Manage Profile avatar selector')
    }
    profileAvatarSelector.setAttribute('columns', kManageProfilePickerColumns)
  },
})
