// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  ProfileCustomizationAppElement as ProfileCustomizationAppElementChromium,
} from './profile_customization_app-chromium.js'

// Merge onto the upstream type so getHtml.bind(this) in the chromium class
// still type-checks against the HTML template's ProfileCustomizationAppElement.
declare module './profile_customization_app-chromium.js' {
  interface ProfileCustomizationAppElement {
    onNameInputKeydown_(e: KeyboardEvent): void
  }
}

// Subclass so Enter-to-confirm is wired in firstUpdated on the registered
// element. Upstream's customElements.define is patched out; we register this
// subclass instead. Listen on inputElement (the real <input> in the shadow
// root) so the key event is received reliably.
class ProfileCustomizationAppElement extends
    ProfileCustomizationAppElementChromium {
  override firstUpdated() {
    super.firstUpdated()
    this.$.nameInput.inputElement.addEventListener(
      'keydown',
      (e: KeyboardEvent) => this.onNameInputKeydown_(e),
    )
  }

  override onNameInputKeydown_(e: KeyboardEvent) {
    if (e.key !== 'Enter' || this.$.doneButton.disabled) {
      return
    }
    e.preventDefault()
    this.$.doneButton.click()
  }
}

export { ProfileCustomizationAppElement }
export * from './profile_customization_app-chromium.js'

customElements.define(
  ProfileCustomizationAppElement.is,
  ProfileCustomizationAppElement,
)
