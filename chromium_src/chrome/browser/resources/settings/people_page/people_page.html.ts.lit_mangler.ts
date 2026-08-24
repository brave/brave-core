// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Our getStarted page embeds this element as a flat list of rows, not as
// its own section, and doesn't support Google sign-in/sync, so the
// "You and Brave" section title and the sync/Google-services rows don't
// belong there.
mangle((root) => {
  const section = root.querySelector('settings-section')
  if (!section) {
    throw new Error(`[Settings] People page: couldn't find settings-section`)
  }

  const accountSubpageRow = root.querySelector('#account-subpage-row')
  if (!accountSubpageRow) {
    throw new Error(
      `[Settings] People page: couldn't find #account-subpage-row`)
  }
  accountSubpageRow.remove()

  const syncSetup = root.querySelector('#sync-setup')
  if (!syncSetup) {
    throw new Error(`[Settings] People page: couldn't find #sync-setup`)
  }
  syncSetup.remove()

  const googleServices = root.querySelector('#google-services')
  if (!googleServices) {
    throw new Error(`[Settings] People page: couldn't find #google-services`)
  }
  googleServices.remove()

  // Replace settings-section with its children, so it doesn't render its own
  // "You and Brave" section title inside the getStarted page.
  section.replaceWith(...Array.from(section.childNodes))
})

// #profile-row lives inside `${this.shouldLinkToProfileRow_() ? html`...` :
// ''}`, a nested template -- it isn't reachable from the root template above.
mangle((root) => {
  const profileRow = root.querySelector('#profile-row')
  if (!profileRow) {
    throw new Error(`[Settings] People page: couldn't find #profile-row`)
  }
  profileRow.remove()
}, (t) => t.text.includes('id="profile-row"'))

// #manage-google-account and #edit-profile ("Profile name and icon") live
// inside `${this.signinAllowed_ ? html`...` : ''}`, another nested template.
// signinAllowed_ is forced to true by the companion chromium_src/.../
// people_page.ts override -- we set signin.allowed to false, which would
// otherwise hide both rows, but #edit-profile needs to stay.
mangle((root) => {
  const manageGoogleAccount = root.querySelector('#manage-google-account')
  if (!manageGoogleAccount) {
    throw new Error(
      `[Settings] People page: couldn't find #manage-google-account`)
  }
  manageGoogleAccount.remove()

  const editProfile = root.querySelector('#edit-profile')
  if (!editProfile) {
    throw new Error(`[Settings] People page: couldn't find #edit-profile`)
  }
  editProfile.classList.add('first')
}, (t) => t.text.includes('id="manage-google-account"'))
