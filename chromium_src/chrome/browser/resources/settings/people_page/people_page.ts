// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  SettingsPeoplePageElement as SettingsPeoplePageElementChromium
} from './people_page-chromium.js'

class SettingsPeoplePageElement extends SettingsPeoplePageElementChromium {
  // We set signin.allowed to false (see
  // BraveProfileManager::InitProfileUserPrefs), which would otherwise hide
  // both #manage-google-account and #edit-profile ("Profile name and icon").
  // The lit_mangler override for this page already removes
  // #manage-google-account, but #edit-profile needs to stay, so force this
  // true.
  //
  // signinAllowed_ is `protected` on the upstream class, and re-declaring it
  // here (e.g. via `override accessor`) would give it a new declaring class,
  // which breaks the protected-member compatibility check TypeScript does
  // between this class and the upstream one anywhere both are referenced
  // (e.g. the HTMLElementTagNameMap augmentation in the unrenamed upstream
  // people_page.ts still points at the upstream class). Setting it in the
  // constructor instead avoids introducing a new declaration.
  constructor() {
    super()
    this.signinAllowed_ = true
  }
}

export { SettingsPeoplePageElement }
export * from './people_page-chromium.js'

// Register our subclass instead of the upstream class. The matching
// `customElements.define` in upstream people_page.ts is patched out.
customElements.define(
  SettingsPeoplePageElement.is, SettingsPeoplePageElement)
