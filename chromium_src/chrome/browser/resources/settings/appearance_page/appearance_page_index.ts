// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'

import { routes } from '../route.js'
import type { Route } from '../router.js'
import {
  SettingsAppearancePageIndexElement as SettingsAppearancePageIndexElementChromium
} from './appearance_page_index-chromium.js'

// Registers <settings-brave-appearance-tabs> and
// <settings-brave-appearance-sidebar>, which are woven into
// settings-appearance-page-index's template by a lit_mangler override (see
// appearance_page_index.html.ts.lit_mangler.ts).
import '../brave_appearance_page/tabs.js'
import '../brave_appearance_page/sidebar.js'

// Brave's Tabs and Sidebar sections are additional top-level views appended
// (via a lit_mangler override of appearance_page_index.html.ts) into the
// same <cr-view-manager> as the upstream Appearance view, and are shown
// simultaneously with it (see currentRouteChanged below). Each view
// normally gets `position: absolute` while inactive so only one is ever
// visible; override that so all three can stack and be visible together.
injectStyle(SettingsAppearancePageIndexElementChromium, css`
  cr-view-manager [slot=view]:not(.closing) {
    position: initial;
  }
`)

class SettingsAppearancePageIndexElement extends
    SettingsAppearancePageIndexElementChromium {
  override currentRouteChanged(newRoute: Route) {
    // Mirror SearchableViewContainerMixinLit's own currentRouteChanged
    // (which just records the route, for search purposes) instead of
    // calling `super`, since upstream's own currentRouteChanged would also
    // switch to showing only the single 'parent' view.
    this.currentRoute = newRoute

    // Need to wait for currentRouteChanged observers on child views to run
    // first, before switching views.
    queueMicrotask(() => {
      switch (newRoute) {
        case routes.APPEARANCE:
        case routes.BASIC:
          // Switch back to the default views in case they are part of
          // search results.
          this.$.viewManager.switchViews(
              ['parent', 'tabs', 'sidebar'], 'no-animation', 'no-animation')
          break
        case routes.FONTS:
          this.$.viewManager.switchView(
              'fonts', 'no-animation', 'no-animation')
          break
        default:
          // Nothing to do. Other parent elements are responsible for
          // updating the displayed contents.
          break
      }
    })
  }
}

export { SettingsAppearancePageIndexElement }
export * from './appearance_page_index-chromium.js'

// Register the Brave subclass instead of the upstream class. The matching
// `customElements.define` in upstream appearance_page_index.ts is patched
// out (see patches/chrome-browser-resources-settings-appearance_page-
// appearance_page_index.ts.patch).
customElements.define(
    SettingsAppearancePageIndexElement.is, SettingsAppearancePageIndexElement)
