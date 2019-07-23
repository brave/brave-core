/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/new_tab_types'
import { Preferences } from '../api/preferences'

// API
import * as gridAPI from '../api/topSites/grid'
import * as dataFetchAPI from '../api/dataFetch'
import * as bookmarksAPI from '../api/topSites/bookmarks'
import * as dndAPI from '../api/topSites/dnd'
import * as storage from '../storage'

export const newTabReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()

    setImmediate(() => {
      dataFetchAPI.fetchTopSites()
    })
  }

  const startingState = state
  const payload = action.payload
  switch (action.type) {
    case types.NEW_TAB_TEXT_DIRECTION_UPDATED:
      state = { ...state, textDirection: payload }
      break
    case types.BOOKMARK_ADDED:
      const topSite: NewTab.Site | undefined = state.topSites.find((site) => site.url === payload.url)
      if (topSite) {
        chrome.bookmarks.create({
          title: topSite.title,
          url: topSite.url
        }, () => {
          bookmarksAPI.fetchBookmarkInfo(payload.url)
        })
      }
      break
    case types.BOOKMARK_REMOVED:
      const bookmarkInfo = state.bookmarks[payload.url]
      if (bookmarkInfo) {
        chrome.bookmarks.remove(bookmarkInfo.id, () => {
          bookmarksAPI.fetchBookmarkInfo(payload.url)
        })
      }
      break
    case types.NEW_TAB_TOP_SITES_DATA_UPDATED:
      state = { ...state, topSites: payload.topSites }
      gridAPI.calculateGridSites(state)
      break

    case types.NEW_TAB_SITE_PINNED: {
      const topSiteIndex: number = state.topSites.findIndex((site) => site.url === payload.url)
      const pinnedTopSite: NewTab.Site = Object.assign({}, state.topSites[topSiteIndex], { pinned: true })
      const pinnedTopSites: NewTab.Site[] = state.pinnedTopSites.slice()

      pinnedTopSite.index = topSiteIndex
      pinnedTopSites.push(pinnedTopSite)
      pinnedTopSites.sort((x, y) => x.index - y.index)
      state = {
        ...state,
        pinnedTopSites
      }
      gridAPI.calculateGridSites(state)
      break
    }

    case types.NEW_TAB_SITE_UNPINNED:
      const currentPositionIndex: number = state.pinnedTopSites.findIndex((site) => site.url === payload.url)
      if (currentPositionIndex !== -1) {
        const pinnedTopSites: NewTab.Site[] = state.pinnedTopSites.slice()
        pinnedTopSites.splice(currentPositionIndex, 1)
        state = {
          ...state,
          pinnedTopSites
        }
      }
      gridAPI.calculateGridSites(state)
      break

    case types.NEW_TAB_SITE_IGNORED: {
      const topSiteIndex: number = state.topSites.findIndex((site) => site.url === payload.url)
      const ignoredTopSites: NewTab.Site[] = state.ignoredTopSites.slice()
      ignoredTopSites.push(state.topSites[topSiteIndex])
      state = {
        ...state,
        ignoredTopSites,
        showSiteRemovalNotification: true
      }
      gridAPI.calculateGridSites(state)
      break
    }

    case types.NEW_TAB_UNDO_SITE_IGNORED: {
      const ignoredTopSites: NewTab.Site[] = state.ignoredTopSites.slice()
      ignoredTopSites.pop()
      state = {
        ...state,
        ignoredTopSites,
        showSiteRemovalNotification: false
      }
      gridAPI.calculateGridSites(state)
      break
    }

    case types.NEW_TAB_UNDO_ALL_SITE_IGNORED:
      state = {
        ...state,
        ignoredTopSites: [],
        showSiteRemovalNotification: false
      }
      gridAPI.calculateGridSites(state)
      break

    case types.NEW_TAB_HIDE_SITE_REMOVAL_NOTIFICATION:
      state = {
        ...state,
        showSiteRemovalNotification: false
      }
      break

    case types.NEW_TAB_SITE_DRAGGED:
      state = dndAPI.onDraggedSite(state, payload.fromUrl, payload.toUrl)
      break

    case types.NEW_TAB_SITE_DRAG_END:
      state = dndAPI.onDragEnd(state)
      break

    case types.NEW_TAB_BOOKMARK_INFO_AVAILABLE:
      state = bookmarksAPI.updateBookmarkInfo(state, payload.queryUrl, payload.bookmarkTreeNode)
      break

    case types.NEW_TAB_GRID_SITES_UPDATED:
      state = { ...state, gridSites: payload.gridSites }
      break

    case types.NEW_TAB_STATS_UPDATED:
      state = storage.getLoadTimeData(state)
      break

    case types.NEW_TAB_USE_ALTERNATIVE_PRIVATE_SEARCH_ENGINE:
      chrome.send('toggleAlternativePrivateSearchEngine', [])
      state = { ...state, useAlternativePrivateSearchEngine: payload.shouldUse }
      break

    case types.NEW_TAB_PREFERENCES_UPDATED:
      const preferences: Preferences = payload.preferences
      state = {
        ...state,
        ...preferences
      }
      break

    default:
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default newTabReducer
