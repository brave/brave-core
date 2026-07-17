// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ProfileCustomizationAppElement } from './profile_customization_app-chromium.js'

// Let users press Enter in the profile name field to confirm profile creation,
// mirroring a click on the "Done" button (but only while it is enabled). The
// handler is attached to #nameInput via the accompanying lit_mangler. We defer
// entirely to the button so behaviour (including the disabled state) stays in
// sync with upstream.
declare module './profile_customization_app-chromium.js' {
  interface ProfileCustomizationAppElement {
    onNameInputKeydown_: (e: KeyboardEvent) => void
  }
}

ProfileCustomizationAppElement.prototype.onNameInputKeydown_ = function (
  this: ProfileCustomizationAppElement,
  e: KeyboardEvent,
) {
  if (e.key !== 'Enter' || this.$.doneButton.disabled) {
    return
  }
  this.$.doneButton.click()
}

export * from './profile_customization_app-chromium.js'
