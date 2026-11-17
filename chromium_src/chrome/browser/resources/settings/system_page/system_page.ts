// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../controls/settings_dropdown_menu.js'

// Used by renderScreenshotShortcutBehavior_().
// eslint-disable-next-line @webui-eslint/lit-element-structure
import { html } from '//resources/lit/v3_0/lit.rollup.js'
import type { TemplateResult } from '//resources/lit/v3_0/lit.rollup.js'

import { isMac } from '//resources/js/platform.js'

import type { DropdownMenuOptionList } from '../controls/settings_dropdown_menu.js'
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
    isBraveScreenshotEnabled_: boolean
    screenshotShortcutBehaviorOptions_: DropdownMenuOptionList
    renderScreenshotShortcutBehavior_(): TemplateResult
    onShortcutsClick_(): void
  }
}

// Must match ScreenshotShortcutBehavior in components/constants/pref_names.h.
enum ScreenshotShortcutBehavior {
  SHOW_OPTIONS = 0,
  SELECTED_AREA = 1,
  VISIBLE_AREA = 2,
  FULL_PAGE = 3,
}

// Upstream's HTMLElementTagNameMap entry for this tag points at the upstream
// class, and TypeScript rejects a second entry with a different type.
// eslint-disable-next-line @webui-eslint/lit-element-structure
class SettingsSystemPageElement extends SettingsSystemPageElementChromium {
  static override get is() {
    return 'settings-system-page'
  }

  static override get properties() {
    return {
      ...super.properties,
      shortcutsSupported_: { type: Boolean },
      isMac_: { type: Boolean },
      warnBeforeQuittingLabel_: { type: String },
      isBraveScreenshotEnabled_: { type: Boolean },
      screenshotShortcutBehaviorOptions_: { type: Array },
    }
  }

  override accessor isBraveScreenshotEnabled_: boolean =
      loadTimeData.getBoolean('isBraveScreenshotEnabled')

  override accessor screenshotShortcutBehaviorOptions_:
      DropdownMenuOptionList = [
    {
      value: ScreenshotShortcutBehavior.SHOW_OPTIONS,
      name: loadTimeData.getString(
          'braveScreenshotShortcutBehaviorShowOptions'),
    },
    {
      value: ScreenshotShortcutBehavior.SELECTED_AREA,
      name: loadTimeData.getString(
          'braveScreenshotShortcutBehaviorSelectedArea'),
    },
    {
      value: ScreenshotShortcutBehavior.VISIBLE_AREA,
      name: loadTimeData.getString(
          'braveScreenshotShortcutBehaviorVisibleArea'),
    },
    {
      value: ScreenshotShortcutBehavior.FULL_PAGE,
      name: loadTimeData.getString('braveScreenshotShortcutBehaviorFullPage'),
    },
  ]

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

  // Rendered here because lit_mangler lowercases the .menuOptions binding.
  override renderScreenshotShortcutBehavior_(): TemplateResult {
    return html`
      <settings-dropdown-menu
          pref-key="brave.screenshot.shortcut_behavior"
          label="${loadTimeData.getString('braveScreenshotShortcutBehavior')}"
          .menuOptions="${this.screenshotShortcutBehaviorOptions_}">
      </settings-dropdown-menu>`
  }

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
