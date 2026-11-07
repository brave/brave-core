// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { routes } from '../route.js'
import type { Route } from '../router.js'
import {
  SettingsSearchPageIndexElement as SettingsSearchPageIndexElementChromium
} from './search_page_index-chromium.js'

class SettingsSearchPageIndexElement extends
    SettingsSearchPageIndexElementChromium {
  // DEFAULT_SEARCH and PRIVATE_SEARCH are Brave-only routes (deep links into
  // the default/private search engine dialogs, which live on the 'parent'
  // view). Just show 'parent' and let settings-search-page's own
  // currentRouteChanged react to the route to open its dialog; upstream's
  // switch-on-route logic (in super.currentRouteChanged) doesn't know about
  // either route.
  override currentRouteChanged(newRoute: Route, oldRoute?: Route) {
    if (newRoute === routes.DEFAULT_SEARCH ||
        newRoute === routes.PRIVATE_SEARCH) {
      this.$.viewManager.switchView('parent', 'no-animation', 'no-animation')
      return
    }
    super.currentRouteChanged(newRoute, oldRoute)
  }
}

export { SettingsSearchPageIndexElement }
export * from './search_page_index-chromium.js'

// Register the Brave subclass instead of the upstream class. The matching
// `customElements.define` in upstream search_page_index.ts is patched out.
customElements.define(
    SettingsSearchPageIndexElement.is, SettingsSearchPageIndexElement)
