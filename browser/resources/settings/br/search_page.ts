/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '../brave_search_engines_page/brave_search_engines_page.js'
import '../brave_search_engines_page/normal_search_engine_list_dialog.js'
import '../brave_search_engines_page/private_search_engine_list_dialog.js'

import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'
import type { SettingsPrefsElement } from '/shared/settings/prefs/prefs.js'

import { loadTimeData } from '../i18n_setup.js'
import { routes } from '../route.js'
import { Router } from '../router.js'
import type { Route } from '../router.js'
import { SettingsSearchPageElement } from '../search_page/search_page.js'
import type { SearchEngine } from '../search_page/search_engines_browser_proxy.js'

// This is deliberately *not* an intersection with SettingsSearchPageElement:
// several of these members (showSearchEngineListDialog_, searchEngines_,
// onSearchEngineChanged_, etc.) are `protected` on the real class, and
// intersecting would keep that restriction alive here. Going through
// `unknown` gives this file its own unrestricted view of the prototype,
// same as the about_page.ts firstUpdated() patch does for `firstUpdated`.
interface PatchableSearchPage {
  shadowRoot: ShadowRoot|null
  showSearchEngineListDialog_: boolean
  searchEngines_: SearchEngine[]
  searchSettingsUpdateEnabled_: boolean
  onSearchEngineChanged_: (e: CustomEvent<{searchEngine: SearchEngine}>) => void
  currentRouteChanged: (newRoute: Route, oldRoute?: Route) => void
}

const proto = SettingsSearchPageElement.prototype as unknown as
    PatchableSearchPage&{
      onOpenDialogButtonClick_: () => void
      onSearchEngineListDialogClose_: () => void
      connectedCallback: () => void
      disconnectedCallback: () => void
      updated: (changed: PropertyValues) => void
    }

// Make the default search dialog navigable via deep linking.
proto.onOpenDialogButtonClick_ = function(this: PatchableSearchPage) {
  Router.getInstance().navigateTo(routes.DEFAULT_SEARCH)
}

proto.onSearchEngineListDialogClose_ = function(this: PatchableSearchPage) {
  Router.getInstance().navigateTo(routes.SEARCH)
}

proto.currentRouteChanged = function(this: PatchableSearchPage) {
  this.showSearchEngineListDialog_ =
    Router.getInstance().getCurrentRoute() === routes.DEFAULT_SEARCH
}

// Emulates what RouteObserverMixin's connectedCallback/disconnectedCallback
// would have added.
const originalConnectedCallback = proto.connectedCallback
proto.connectedCallback = function(this: PatchableSearchPage) {
  originalConnectedCallback.call(this)
  const router = Router.getInstance()
  router.addObserver(this)
  proto.currentRouteChanged.call(this, router.getCurrentRoute())
}

const originalDisconnectedCallback = proto.disconnectedCallback
proto.disconnectedCallback = function(this: PatchableSearchPage) {
  originalDisconnectedCallback.call(this)
  Router.getInstance().removeObserver(this)
}

// Finds the global settings-prefs singleton, which lives in settings-ui's
// shadow root. Needed because settings-brave-search-page still uses the
// classic PrefsMixin, which requires the whole prefs tree to be handed to it
// as a property.
function getPrefsElement(): SettingsPrefsElement|null {
  return document.querySelector('settings-ui')
             ?.shadowRoot?.querySelector('settings-prefs') ?? null
}

function modifySearchPage(page: PatchableSearchPage) {
  const root = page.shadowRoot
  if (!root) {
    return
  }

  // Replace the search engine list dialog with Brave's own version, which
  // adds the Brave-promoted engine and the guest-mode "save choice" checkbox.
  let braveDialog =
      root.querySelector('settings-normal-search-engine-list-dialog')
  if (page.showSearchEngineListDialog_) {
    if (!braveDialog) {
      braveDialog =
          document.createElement('settings-normal-search-engine-list-dialog')
      braveDialog.addEventListener(
          'close', () => proto.onSearchEngineListDialogClose_.call(page))
      braveDialog.addEventListener(
          'search-engine-changed',
          (e: Event) => page.onSearchEngineChanged_(
              e as CustomEvent<{searchEngine: SearchEngine}>))

      const searchEngineListDialog =
          root.querySelector('settings-search-engine-list-dialog')
      if (searchEngineListDialog) {
        searchEngineListDialog.replaceWith(braveDialog)
      } else {
        console.error(`[Settings] Couldn't find search engine list dialog`)
        root.appendChild(braveDialog)
      }
    }
    braveDialog.searchEngines = page.searchEngines_
  } else if (braveDialog) {
    braveDialog.remove()
  }

  const searchEngineTitleElement = root.querySelector('.default-search-engine')
  if (searchEngineTitleElement?.firstChild?.nodeType === Node.TEXT_NODE) {
    searchEngineTitleElement.firstChild.textContent =
      loadTimeData.getString('normalSearchEnginesSiteSearchEngineHeading')
  } else {
    console.error(`[Settings] Couldn't find search engine title text node`)
  }

  if (!page.searchSettingsUpdateEnabled_ &&
      !root.querySelector('settings-brave-search-page')) {
    const enginesSubpageTrigger = root.getElementById('enginesSubpageTrigger')
    if (!enginesSubpageTrigger) {
      console.error(`[Settings] Couldn't find enginesSubpageTrigger`)
    } else {
      const bravePage =
          document.createElement('settings-brave-search-page') as
          HTMLElement&{prefs: Record<string, unknown>}
      enginesSubpageTrigger.insertAdjacentElement('beforebegin', bravePage)

      const prefsElement = getPrefsElement()
      if (!prefsElement) {
        console.error(`[Settings] Couldn't find the settings-prefs singleton`)
      } else {
        // `prefs` is undefined until CrSettingsPrefs finishes initializing,
        // so only forward it once it's actually populated.
        if (prefsElement.prefs) {
          bravePage.prefs = prefsElement.prefs
        }
        prefsElement.addEventListener('prefs-changed', () => {
          if (prefsElement.prefs) {
            bravePage.prefs = prefsElement.prefs
          }
        })
      }
    }
  }
}

const originalUpdated = proto.updated
proto.updated = function(this: PatchableSearchPage, changed: PropertyValues) {
  originalUpdated?.call(this, changed)
  modifySearchPage(this)
}
