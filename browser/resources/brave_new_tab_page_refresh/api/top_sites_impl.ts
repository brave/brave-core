/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { NewTabPageProxy } from './new_tab_page_proxy'
import { TopSitesAPI, TopSitesListKind, defaultTopSitesState } from './top_sites_api'
import { createStore } from '../lib/store'
import { debounceListener } from '../lib/debounce_listener'

export function createTopSitesAPI(): TopSitesAPI {
  const store = createStore(defaultTopSitesState())

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
      topSitesListKind: listKind
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

    getState: store.getState,

    addListener: store.addListener,

    setShowTopSites(showTopSites) {
      handler.setShowTopSites(showTopSites)
    },

    setTopSitesListKind(listKind) {
      handler.setTopSitesListKind(listKind)
    },

    addTopSite(url, title) {
      handler.addCustomTopSite(url, title)
    },

    updateTopSite(currentURL, newURL, title) {
      handler.updateCustomTopSite(currentURL, newURL, title)
    },

    removeTopSite(url) {
      if (currentListKind() === TopSitesListKind.kMostVisited) {
        handler.excludeMostVisitedTopSite(url)
        lastExcludedMostVisitedSite = url
      } else {
        handler.removeCustomTopSite(url)
      }
    },

    undoRemoveTopSite() {
      if (currentListKind() === TopSitesListKind.kMostVisited) {
        if (lastExcludedMostVisitedSite) {
          handler.includeMostVisitedTopSite(lastExcludedMostVisitedSite)
          lastExcludedMostVisitedSite = ''
        }
      } else {
        handler.undoCustomTopSiteAction()
      }
    },

    setTopSitePosition(url, pos) {
      if (currentListKind() === TopSitesListKind.kCustom) {
        handler.setCustomTopSitePosition(url, pos)
      }
    }
  }
}
