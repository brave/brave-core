// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../brave_search_engines_page/brave_search_engines_page.js'
import '../brave_search_engines_page/normal_search_engine_list_dialog.js'
// settings-brave-search-page's own template references this, but doesn't
// import it itself -- it relies on whoever registers it to also pull this in.
import '../brave_search_engines_page/private_search_engine_list_dialog.js'

import { CrSettingsPrefs } from '/shared/settings/prefs/prefs_types.js'
import type { SettingsPrefsElement } from '/shared/settings/prefs/prefs.js'
import type { Route } from '../router.js'

import { routes } from '../route.js'
import { Router } from '../router.js'
import {
  SettingsSearchPageElement as SettingsSearchPageElementChromium
} from './search_page-chromium.js'

// Declaration-merge the Brave-only member onto the upstream class type so the
// lit_mangler-injected template (typed with `this: SettingsSearchPageElement`
// via the upstream search_page.html.ts import) type-checks.
declare module './search_page-chromium.js' {
  interface SettingsSearchPageElement {
    bravePrefs_: { [key: string]: unknown }|undefined
    onPrefsChanged_: () => void
  }
}

// Finds the global settings-prefs singleton, which lives in settings-ui's
// shadow root. Needed because settings-brave-search-page still uses the
// classic PrefsMixin, which requires the whole prefs tree to be handed to it
// as a property, and the now-Lit search page has no `prefs` property of its
// own to forward.
function getPrefsElement(): SettingsPrefsElement|null {
  return document.querySelector('settings-ui')
             ?.shadowRoot?.querySelector('settings-prefs') ?? null
}

class SettingsSearchPageElement extends SettingsSearchPageElementChromium {
  static override get properties() {
    return {
      ...super.properties,
      bravePrefs_: { type: Object },
    }
  }

  // Left undefined until real prefs arrive, so the lit_mangler-injected
  // template (see the companion search_page.html.ts override) can gate
  // rendering settings-brave-search-page on it: that element's children are
  // still Polymer and bind to specific pref paths (e.g. `pref="{{prefs.foo}}"`)
  // via the classic PrefsMixin, which logs a "Pref error [not found]" console
  // error the instant they connect with an empty/incomplete `prefs` object.
  override accessor bravePrefs_: { [key: string]: unknown }|undefined =
      undefined

  override onPrefsChanged_ = () => {
    const prefsElement = getPrefsElement()
    if (prefsElement?.prefs) {
      this.bravePrefs_ = prefsElement.prefs
    }
  }

  override connectedCallback() {
    super.connectedCallback()

    const prefsElement = getPrefsElement()
    if (!prefsElement) {
      console.error(`[Settings] Couldn't find the settings-prefs singleton`)
      return
    }
    CrSettingsPrefs.initialized.then(() => {
      this.bravePrefs_ = prefsElement.prefs
    })
    prefsElement.addEventListener('prefs-changed', this.onPrefsChanged_)
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    getPrefsElement()?.removeEventListener(
        'prefs-changed', this.onPrefsChanged_)
  }

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
