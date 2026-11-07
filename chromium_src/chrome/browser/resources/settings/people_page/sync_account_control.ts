// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'

import {
  SettingsSyncAccountControlElement as SettingsSyncAccountControlElementChromium
} from './sync_account_control-chromium.js'

// We don't support Google sign-in/sync, so this promo banner and the
// avatar row (both pointing at Google account flows we don't offer) don't
// belong anywhere we embed <settings-sync-account-control> (e.g. the
// getStarted page).
class SettingsSyncAccountControlElement extends
    SettingsSyncAccountControlElementChromium {
  override willUpdate(changedProperties: PropertyValues<this>) {
    super.willUpdate(changedProperties)
    this.shouldShowAvatarRow_ = false
  }
}

// shouldHideBanner_ is `protected` on the upstream class. Overriding it as a
// real class member would give it a new declaring class, which breaks the
// protected-member compatibility check TypeScript does between this class
// and the upstream one anywhere both are referenced (e.g. the
// HTMLElementTagNameMap augmentation in the unrenamed upstream
// sync_account_control.ts still points at the upstream class). Patching the
// prototype from outside the class body sidesteps that.
const proto = SettingsSyncAccountControlElement.prototype as unknown as {
  shouldHideBanner_: () => boolean
}

proto.shouldHideBanner_ = function() {
  return true
}

export { SettingsSyncAccountControlElement }
export * from './sync_account_control-chromium.js'

// Register our subclass instead of the upstream class. The matching
// `customElements.define` in upstream sync_account_control.ts is patched out
customElements.define(
    SettingsSyncAccountControlElement.is, SettingsSyncAccountControlElement)
