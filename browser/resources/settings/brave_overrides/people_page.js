// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {RegisterStyleOverride, RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'

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
    const page = templateContent.querySelector('settings-animated-pages[section=people]')
    page.setAttribute('section', 'getStarted')
    // The 'Manage profile' button is inside the "signin-allowed" conditional template.
    // We don't allow Google Sign-in, but we do allow local profile editing, so we have to turn
    // the template back on and remove the google signin prompt.
    const signinTemplate = templateContent.querySelector('template[is=dom-if][if="[[signinAllowed_]]"]')
    if (!signinTemplate) {
      console.error('[Brave Settings Overrides] People Page cannot find signin template')
      return
    }
    const syncSetupLink = templateContent.querySelector('#sync-setup')
    if (syncSetupLink) {
      syncSetupLink.remove()
    } else {
      console.error('[Brave Settings Overrides] People Page cannot find sync-setup link')
    }
    const syncSetup = templateContent.querySelector('template[is=dom-if][route-path="/syncSetup"]')
    if (syncSetup) {
      syncSetup.remove()
    } else {
      console.error('[Brave Settings Overrides] People Page cannot find syncSetup template')
    }
    const syncSetupAdvanced = templateContent.querySelector('template[is=dom-if][route-path="/syncSetup/advanced"]')
    if (syncSetupAdvanced) {
      syncSetupAdvanced.remove()
    } else {
      console.error('[Brave Settings Overrides] People Page cannot find syncSetup/advanced template')
    }
    // always show the template content
    signinTemplate.setAttribute('if', 'true')
    // remove the google account button
    const manageGoogleAccount = signinTemplate.content.querySelector('#manage-google-account')
    if (!manageGoogleAccount) {
      console.error('[Brave Settings Overrides] Could not find the google account settings item', templateContent, templateContent.textContent)
      return
    }
    manageGoogleAccount.remove()
    // Edit profile item needs to know it's the first in the section
    const firstItem = signinTemplate.content.querySelector('#edit-profile')
    if (!firstItem) {
      console.error('[Brave Settings Overrides] Could not find #edit-profile item in people_page')
      return
    }
    firstItem.classList.add('first')
  },
})
