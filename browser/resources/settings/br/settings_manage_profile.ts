// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerTemplateModifications,
  RegisterStyleOverride,
} from 'chrome://resources/brave/polymer_overriding.js'
import {html as polymerHtml} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

// <if expr="enable_custom_profile_image">
import 'chrome://resources/brave/custom_profile_image_row.js'

import {html, render} from 'chrome://resources/lit/v3_0/lit.rollup.js'

import {BraveSettingsStrings} from '../brave_generated_resources_webui_strings.js'
import {loadTimeData} from '../i18n_setup.js'
// </if>

// Brave avatar assets are organized in 7-color groups per style under
// `app/theme/default_100_percent/common/avatars`. Chromium's default 6-column
// picker splits each group across multiple rows. Force the use of 7 columns to
// keep variants visually aligned.
const kManageProfilePickerColumns = '7'

RegisterStyleOverride(
  'settings-manage-profile',
  polymerHtml`
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

      .custom-profile-image-section .content {
        --icon-grid-gap: 22px;
        --icon-size: 66px;
      }
    </style>
  `,
)

// <if expr="enable_custom_profile_image">
const kCustomProfileImageRowId = 'customProfileImageRow'

function createCustomProfileImageSection(): DocumentFragment {
  const section = document.createDocumentFragment()
  render(
    html`
      <div
        class="cr-row manage-profile-section custom-profile-image-section"
      >
        <h1 class="cr-title-text">
          ${loadTimeData.getString(
            BraveSettingsStrings.CUSTOM_PROFILE_IMAGE_TITLE,
          )}
        </h1>
        <div class="content">
          <br-custom-profile-image-row
            id=${kCustomProfileImageRowId}
            hide-title
            title-label=${loadTimeData.getString(
              BraveSettingsStrings.CUSTOM_PROFILE_IMAGE_UPLOAD_ACTION,
            )}
            replace-label=${loadTimeData.getString(
              BraveSettingsStrings.CUSTOM_PROFILE_IMAGE_REPLACE_ACTION,
            )}
            replace-tooltip=${loadTimeData.getString(
              BraveSettingsStrings.CUSTOM_PROFILE_IMAGE_REPLACE_TOOLTIP,
            )}
            remove-label=${loadTimeData.getString(
              BraveSettingsStrings.CUSTOM_PROFILE_IMAGE_REMOVE_ACTION,
            )}
            selected-preview-label=${loadTimeData.getString(
              BraveSettingsStrings.CUSTOM_PROFILE_IMAGE_SELECTED_PREVIEW_LABEL,
            )}
            upload-tooltip=${loadTimeData.getString(
              BraveSettingsStrings.CUSTOM_PROFILE_IMAGE_UPLOAD_TOOLTIP,
            )}
            remove-tooltip=${loadTimeData.getString(
              BraveSettingsStrings.CUSTOM_PROFILE_IMAGE_REMOVE_TOOLTIP,
            )}
            invalid-image-label=${loadTimeData.getString(
              BraveSettingsStrings.CUSTOM_PROFILE_IMAGE_INVALID_IMAGE,
            )}
          ></br-custom-profile-image-row>
        </div>
      </div>
    `,
    section,
  )
  return section
}
// </if>

function customizeManageProfileTemplate(templateContent: DocumentFragment) {
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

  // <if expr="enable_custom_profile_image">
  if (!loadTimeData.getBoolean('customProfileImageEnabled')) {
    return
  }

  if (templateContent.querySelector(`#${kCustomProfileImageRowId}`)) {
    return
  }

  const themeColorSection = themeColorPicker.closest(
    '.manage-profile-section',
  )
  if (!themeColorSection) {
    throw new Error(
      '[Settings] Missing Manage Profile theme color picker section',
    )
  }

  const profileAvatarSection = profileAvatarSelector.closest(
    '.manage-profile-section',
  )
  if (!profileAvatarSection) {
    throw new Error('[Settings] Missing Manage Profile avatar section')
  }

  const sectionParent = profileAvatarSection.parentElement
  if (themeColorSection.parentElement !== sectionParent) {
    throw new Error('[Settings] Manage Profile sections changed structure')
  }

  sectionParent!.insertBefore(
    createCustomProfileImageSection(),
    profileAvatarSection,
  )
  // </if>
}

RegisterPolymerTemplateModifications({
  'settings-manage-profile': customizeManageProfileTemplate,
})
