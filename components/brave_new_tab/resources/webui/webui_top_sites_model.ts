/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_new_tab/new_tab_page.mojom.m.js'

import { NewTabPageProxy } from './new_tab_page_proxy'
import { TopSitesModel, TopSitesListKind, defaultState } from '../models/top_sites_model'
import { createStore } from '../lib/store'
import { debounce } from '$web-common/debounce'

export function listKindFromMojo(type: number): TopSitesListKind {
  switch (type) {
    case mojom.TopSitesListKind.kCustom: return 'custom'
    case mojom.TopSitesListKind.kMostVisited: return 'most-visited'
    default: return 'most-visited'
  }
}

export function createTopSitesModel(): TopSitesModel {
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  const store = createStore(defaultState())
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
      listKind: listKindFromMojo(listKind)
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

  newTabProxy.addListeners({
    onTopSitesPrefsUpdated: debounce(loadData, 10),
    onTopSitesListUpdated: () => loadData()
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

    setListKind(listKind) {
      handler.setTopSitesListKind(listKind === 'custom'
        ? mojom.TopSitesListKind.kCustom
        : mojom.TopSitesListKind.kMostVisited)
    },

    async addTopSite(url, title) {
      await handler.addCustomTopSite(url, title)
    },

    async updateTopSite(currentURL, newURL, title) {
      await handler.updateCustomTopSite(currentURL, newURL, title)
    },

    async removeTopSite(url) {
      const { listKind } = store.getState()
      if (listKind === 'most-visited') {
        await handler.excludeMostVisitedTopSite(url)
        lastExcludedMostVisitedSite = url
      } else {
        await handler.removeCustomTopSite(url)
      }
    },

    async undoRemoveTopSite() {
      const { listKind } = store.getState()
      if (listKind === 'most-visited') {
        if (lastExcludedMostVisitedSite) {
          await handler.includeMostVisitedTopSite(lastExcludedMostVisitedSite)
          lastExcludedMostVisitedSite = ''
        }
      } else {
        await handler.undoCustomTopSiteAction()
      }
    },

    async setTopSitePosition(url, pos) {
      if (store.getState().listKind === 'custom') {
        await handler.setCustomTopSitePosition(url, pos)
      }
    },
  }
}
