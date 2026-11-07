// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { isMac } from '//resources/js/platform.js'

import { loadTimeData } from '../i18n_setup.js'
import { routes } from '../route.js'
import { Router } from '../router.js'

import {
  SettingsSystemPageElement as SettingsSystemPageElementChromium
} from './system_page-chromium.js'

// Declaration-merge the Brave-only members onto the upstream class type so
// the lit_mangler-injected template (typed with `this: SettingsSystemPageElement`
// via the upstream system_page.html.ts import) type-checks.
declare module './system_page-chromium.js' {
  interface SettingsSystemPageElement {
    shortcutsSupported_: boolean
    isMac_: boolean
    warnBeforeQuittingLabel_: string
    onShortcutsClick_(): void
  }
}

class SettingsSystemPageElement extends SettingsSystemPageElementChromium {
  static override get properties() {
    return {
      ...super.properties,
      shortcutsSupported_: { type: Boolean },
      isMac_: { type: Boolean },
      warnBeforeQuittingLabel_: { type: String },
    }
  }

  override accessor shortcutsSupported_: boolean =
      loadTimeData.getBoolean('areShortcutsSupported')
  override accessor isMac_: boolean = isMac

  // 'warnBeforeQuitting' is only registered as a localized string on mac
  // (see settings_localized_strings_provider.cc), so this must not be read
  // unconditionally: doing so would CHECK-fail on every other platform. The
  // `isMac` guard (evaluated at field-initialization time, not a reactive
  // read) keeps it from ever calling into loadTimeData outside of mac.
  override accessor warnBeforeQuittingLabel_: string =
      isMac ? loadTimeData.getString('warnBeforeQuitting') : ''

  override onShortcutsClick_() {
    Router.getInstance().navigateTo(routes.SHORTCUTS)
  }
}

export { SettingsSystemPageElement }
export * from './system_page-chromium.js'

// Register the Brave subclass instead of the upstream class. The matching
// `customElements.define` in upstream system_page.ts is patched out (see
// patches/chrome-browser-resources-settings-system_page-system_page.ts.patch).
customElements.define(
    SettingsSystemPageElement.is, SettingsSystemPageElement)
