/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.m.js'

import { NewTabPageProxy } from './new_tab_page_proxy'
import { TopSitesState, TopSitesActions, TopSitesListKind } from '../models/top_sites'
import { Store } from '../lib/store'
import { debounceListener } from './debounce_listener'

export function listKindFromMojo(type: number): TopSitesListKind {
  switch (type) {
    case mojom.TopSitesListKind.kCustom: return 'custom'
    case mojom.TopSitesListKind.kMostVisited: return 'most-visited'
    default: return 'most-visited'
  }
}

export function initializeTopSites(
    store: Store<TopSitesState>): TopSitesActions {
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  let lastExcludedMostVisitedSite = ''

  async function updatePrefs() {
    const [
      { showTopSites },
      { listKind }
    ] = await Promise.all([
      handler.getShowTopSites(),
      handler.getTopSitesListKind()
    ])

    store.update({
      showTopSites,
      topSitesListKind: listKindFromMojo(listKind)
    })
  }

  async function updateTopSites() {
    const { topSites } = await handler.getTopSites()
    store.update({ topSites })
  }

  async function loadData() {
    await Promise.all([
      updateTopSites(),
      updatePrefs()
    ])
  }

  function currentListKind() {
    return store.getState().topSitesListKind
  }

  newTabProxy.addListeners({
    onTopSitesUpdated: debounceListener(loadData)
  })

  document.addEventListener('visibilitychange', () => {
    if (document.visibilityState === 'visible') {
      updateTopSites()
    }
  })

  loadData()

  return {

    setShowTopSites(showTopSites) {
      handler.setShowTopSites(showTopSites)
    },

    setTopSitesListKind(listKind) {
      handler.setTopSitesListKind(listKind === 'custom'
          ? mojom.TopSitesListKind.kCustom
          : mojom.TopSitesListKind.kMostVisited)
    },

    addTopSite(url, title) {
      handler.addCustomTopSite(url, title)
    },

    updateTopSite(currentURL, newURL, title) {
      handler.updateCustomTopSite(currentURL, newURL, title)
    },

    removeTopSite(url) {
      if (currentListKind() === 'most-visited') {
        handler.excludeMostVisitedTopSite(url)
        lastExcludedMostVisitedSite = url
      } else {
        handler.removeCustomTopSite(url)
      }
    },

    undoRemoveTopSite() {
      if (currentListKind() === 'most-visited') {
        if (lastExcludedMostVisitedSite) {
          handler.includeMostVisitedTopSite(lastExcludedMostVisitedSite)
          lastExcludedMostVisitedSite = ''
        }
      } else {
        handler.undoCustomTopSiteAction()
      }
    },

    setTopSitePosition(url, pos) {
      if (currentListKind() === 'custom') {
        handler.setCustomTopSitePosition(url, pos)
      }
    },
  }
}
