// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'

import { SettingsManageProfileElement } from '../people_page/manage_profile.js'

// <if expr="enable_custom_profile_image">
import 'chrome://resources/brave/custom_profile_image_row.js'

import { getTrustedHTML } from 'chrome://resources/js/static_types.js'

import { CustomProfileImageStrings } from '../brave_generated_resources_webui_strings.js'
import { loadTimeData } from '../i18n_setup.js'
// </if>

// Brave avatar assets are organized in 7-color groups per style under
// `app/theme/default_100_percent/common/avatars`. Chromium's default 6-column
// picker splits each group across multiple rows. Force the use of 7 columns to
// keep variants visually aligned.
const kManageProfilePickerColumns = '7'

const kPickerSelectors = ['cr-theme-color-picker', 'cr-profile-avatar-selector']

injectStyle(
  SettingsManageProfileElement,
  css`
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
  `,
)

// <if expr="enable_custom_profile_image">
injectStyle(
  SettingsManageProfileElement,
  css`
    .custom-profile-image-section .content {
      --icon-grid-gap: 22px;
      --icon-size: 66px;
    }
  `,
)

function createCustomProfileImageSection() {
  const section = document.createElement('div')
  section.className =
    'cr-row manage-profile-section custom-profile-image-section'
  section.innerHTML = getTrustedHTML`
    <h1 class="cr-title-text"></h1>
    <div class="content">
      <br-custom-profile-image-row hide-title></br-custom-profile-image-row>
    </div>
  `
  section.querySelector('h1')!.textContent = loadTimeData.getString(
    CustomProfileImageStrings.CUSTOM_PROFILE_IMAGE_TITLE,
  )
  return section
}
// </if>

// `firstUpdated` is `protected` on ReactiveElement, so reach it through an
// untyped view of the prototype to patch it from outside the class hierarchy.
const proto = SettingsManageProfileElement.prototype as unknown as {
  shadowRoot: ShadowRoot | null
  firstUpdated?: (changedProperties: PropertyValues) => void
}

const originalFirstUpdated = proto.firstUpdated
proto.firstUpdated = function (
  this: SettingsManageProfileElement,
  changedProperties: PropertyValues,
) {
  originalFirstUpdated?.call(this, changedProperties)

  // The pickers take `columns` as an attribute rendered by the static part of
  // the Lit template, so setting it once after the first render sticks.
  for (const selector of kPickerSelectors) {
    const picker = this.shadowRoot?.querySelector(selector)
    if (!picker) {
      console.error(`[Settings] Missing Manage Profile picker '${selector}'`)
      continue
    }
    picker.setAttribute('columns', kManageProfilePickerColumns)
  }

  // <if expr="enable_custom_profile_image">
  if (!loadTimeData.getBoolean('customProfileImageEnabled')) {
    return
  }

  const themeColorSection = this.shadowRoot
    ?.querySelector('cr-theme-color-picker')
    ?.closest('.manage-profile-section')
  if (!themeColorSection) {
    console.error(
      '[Settings] Missing Manage Profile theme color picker section',
    )
    return
  }

  const profileAvatarSection = this.shadowRoot
    ?.querySelector('cr-profile-avatar-selector')
    ?.closest('.manage-profile-section')
  if (!profileAvatarSection) {
    console.error('[Settings] Missing Manage Profile avatar section')
    return
  }

  const sectionParent = profileAvatarSection.parentElement
  if (themeColorSection.parentElement !== sectionParent) {
    console.error('[Settings] Manage Profile sections changed structure')
    return
  }

  sectionParent!.insertBefore(
    createCustomProfileImageSection(),
    profileAvatarSection,
  )
  // </if>
}
