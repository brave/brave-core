/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import { NewTabPageProxy } from './new_tab_page_proxy'
import {
  TopSitesState,
  TopSitesActions,
  TopSitesListKind,
} from './top_sites_state'
import { Store } from '../lib/store'
import { debounce } from '$web-common/debounce'

export function createTopSitesHandler(
  store: Store<TopSitesState>,
): TopSitesActions {
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  let lastExcludedMostVisitedSite = ''

  store.update({
    maxCustomTopSites: loadTimeData.getInteger('maxCustomTopSites'),
  })

  async function updatePrefs() {
    const [{ showTopSites }, { listKind }] = await Promise.all([
      handler.getShowTopSites(),
      handler.getTopSitesListKind(),
    ])

    store.update({
      showTopSites,
      topSitesListKind: listKind,
    })
  }

  async function updateTopSites() {
    const { topSites } = await handler.getTopSites()
    store.update({ topSites })
  }

  async function loadData() {
    await Promise.all([updateTopSites(), updatePrefs()])

    store.update({ initialized: true })
  }

  function currentListKind() {
    return store.getState().topSitesListKind
  }

  newTabProxy.addListeners({
    onTopSitesUpdated: debounce(loadData, 10),
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
      if (currentListKind() !== TopSitesListKind.kCustom) {
        return
      }

      handler.setCustomTopSitePosition(url, pos)

      store.update(({ topSites }) => {
        const current = topSites.findIndex((item) => item.url === url)
        if (current >= 0) {
          const item = topSites.splice(current, 1)[0]
          topSites.splice(pos, 0, item)
        }
        return { topSites: [...topSites] }
      })
    },
  }
}
