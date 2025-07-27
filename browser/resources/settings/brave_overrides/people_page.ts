/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  RegisterPolymerTemplateModifications,
  RegisterStyleOverride
} from 'chrome://resources/brave/polymer_overriding.js'
import {
  html
} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

RegisterStyleOverride(
  'settings-people-page',
  html`
    <style>
      #profile-row
      {
        display: none !important;
      }
    </style>
  `
)

RegisterPolymerTemplateModifications({
  'settings-people-page': (templateContent) => {
    // People page needs to think it's in the getStarted section, since it is
    // (we remove the People section as a separate section).
//    const page = templateContent.querySelector('settings-animated-pages[section=people]')
//    page.setAttribute('section', 'getStarted')
    // Replace settings-section with its children
    const settingsSection = templateContent.querySelector('settings-section')
    if (settingsSection) {
      settingsSection.replaceWith(...settingsSection.childNodes)
    } else {
      throw new Error('[Settings] Missing settings-section on people_page')
    }

    const syncSetupLink = templateContent.querySelector('#sync-setup')
    if (syncSetupLink) {
      syncSetupLink.remove()
    } else {
      throw new Error('[Settings] Missing sync-setup link on people_page')
    }

    // The 'Manage profile' button is inside the "signin-allowed" conditional
    // template. We don't allow Google Sign-in, but we do allow local profile
    // editing, so we have to turn the template back on and remove the google
    // signin prompt.
    const signinTemplate = templateContent.
      querySelector('template[is=dom-if][if="[[signinAllowed_]]"]')
    if (signinTemplate) {
      // Always show the signin template content
      signinTemplate.setAttribute('if', 'true')
    } else {
      throw new Error('[Settings] Missing signin template on people_page')
    }

    // Remove the google account button
    const manageGoogleAccount =
      signinTemplate.content.querySelector('#manage-google-account')
    if (manageGoogleAccount) {
      manageGoogleAccount.remove()
    } else {
      throw new Error('[Settings] Missing manage-google-account button')
    }

    // Edit profile item needs to know it's the first item in the section
    const firstItem = signinTemplate.content.querySelector('#edit-profile')
    if (firstItem) {
      firstItem.classList.add('first')
    } else {
      throw new Error('[Settings] Missing edit-profile item on people_page')
    }
  },
})
