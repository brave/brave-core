// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../brave_search_engines_page/brave_search_engines_page.js'
import '../brave_search_engines_page/normal_search_engine_list_dialog.js'
// settings-brave-search-page's own template references this, but doesn't
// import it itself -- it relies on whoever registers it to also pull this in.
import '../brave_search_engines_page/private_search_engine_list_dialog.js'

import type { Route } from '../router.js'

import { routes } from '../route.js'
import { Router } from '../router.js'
import {
  SettingsSearchPageElement as SettingsSearchPageElementChromium
} from './search_page-chromium.js'

class SettingsSearchPageElement extends SettingsSearchPageElementChromium {
  override currentRouteChanged(newRoute: Route, oldRoute?: Route) {
    super.currentRouteChanged(newRoute, oldRoute)
    this.showSearchEngineListDialog_ = newRoute === routes.DEFAULT_SEARCH
  }
}

// onOpenDialogButtonClick_ and onSearchEngineListDialogClose_ are `protected`
// on the upstream class. Overriding them as real class members would give
// them a new declaring class, which breaks the protected-member
// compatibility check TypeScript does between this class and the upstream
// one inside search_page-chromium.ts's own `getHtml.bind(this)()` call (its
// `this` is typed as the upstream class, but `getHtml`'s parameter resolves,
// through the unrenamed `search_page.js` import, to this subclass). Patching
// the prototype from outside the class body sidesteps that.
const proto = SettingsSearchPageElement.prototype as unknown as {
  onOpenDialogButtonClick_: () => void
  onSearchEngineListDialogClose_: () => void
}

// Make the default search dialog navigable via deep linking.
proto.onOpenDialogButtonClick_ = function(this: SettingsSearchPageElement) {
  Router.getInstance().navigateTo(routes.DEFAULT_SEARCH)
}

proto.onSearchEngineListDialogClose_ = function(
    this: SettingsSearchPageElement) {
  Router.getInstance().navigateTo(routes.SEARCH)
}

export { SettingsSearchPageElement }
export * from './search_page-chromium.js'

// Register the Brave subclass instead of the upstream class. The matching
// `customElements.define` in upstream search_page.ts is patched out.
customElements.define(
    SettingsSearchPageElement.is, SettingsSearchPageElement)
