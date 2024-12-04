/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStore } from '../lib/store'

import {
  TopSitesModel,
  TopSitesState,
  TopSite,
  defaultModel,
  defaultState } from '../models/top_sites_model'

export function createTopSitesModel(): TopSitesModel {
  let lastRemovedSite: TopSite | null = null

  const store = createStore<TopSitesState>({
    ...defaultState(),

    showTopSites: true,

    listKind: 'custom',

    topSites: [...Array(12).keys()].flatMap((i) => {
      return [
        {
          title: 'Brave',
          favicon: 'https://brave.com/favicon.ico',
          url: `https://brave.com/#${i}`
        },
        {
          title: 'Wikipedia',
          favicon: 'https://en.wikipedia.org/favicon.ico',
          url: `https://en.wikipedia.org/#${i}`
        }
      ]
    })
  })

  return {
    ...defaultModel(),

    getState: store.getState,
    addListener: store.addListener,

    setShowTopSites(showTopSites) {
      store.update({ showTopSites })
    },

    setListKind(listKind) {
      store.update({ listKind })
    },

    addTopSite(url, title) {
      store.update(({ topSites }) => {
        return {
          topSites: [
            ...topSites,
            { url, title, favicon: 'https://brave.com/favicon.ico'}
          ]
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
          })
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
          })
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
    }
  }
}
