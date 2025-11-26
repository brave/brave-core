/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Store } from '../lib/store'

import {
  TopSitesState,
  TopSitesActions,
  TopSite,
  TopSitesListKind,
} from '../state/top_sites_state'

export function createTopSitesHandler(
  store: Store<TopSitesState>,
): TopSitesActions {
  let lastRemovedSite: TopSite | null = null

  store.update({
    initialized: true,

    showTopSites: true,

    topSitesListKind: TopSitesListKind.kCustom,

    topSites: [...Array(11).keys()].flatMap((i) => {
      return [
        {
          title: 'Brave',
          favicon: 'https://brave.com/favicon.ico',
          url: `https://brave.com/#${i}`,
        },
        {
          title: 'Wikipedia',
          favicon: 'https://en.wikipedia.org/favicon.ico',
          url: `https://en.wikipedia.org/#${i}`,
        },
      ]
    }),
  })

  return {
    setShowTopSites(showTopSites) {
      store.update({ showTopSites })
    },

    setTopSitesListKind(listKind) {
      store.update({ topSitesListKind: listKind })
    },

    addTopSite(url, title) {
      store.update(({ topSites }) => {
        return {
          topSites: [
            ...topSites,
            { url, title, favicon: 'https://brave.com/favicon.ico' },
          ],
        }
      })
    },

    updateTopSite(currentURL, newURL, title) {
      store.update(({ topSites }) => {
        return {
          topSites: topSites.map((item) => {
            if (item.url === currentURL) {
              item.url = newURL
              item.title = title
            }
            return item
          }),
        }
      })
    },

    removeTopSite(url) {
      store.update(({ topSites }) => {
        return {
          topSites: topSites.filter((topSite) => {
            if (topSite.url !== url) {
              return true
            }
            lastRemovedSite = topSite
            return false
          }),
        }
      })
    },

    undoRemoveTopSite() {
      if (lastRemovedSite) {
        store.update(({ topSites }) => {
          return { topSites: [...topSites, lastRemovedSite!] }
        })
        lastRemovedSite = null
      }
    },

    setTopSitePosition(url, pos) {
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
