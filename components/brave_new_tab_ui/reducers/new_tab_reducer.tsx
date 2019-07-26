/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/new_tab_types'
import { Preferences } from '../api/preferences'
import { Stats } from '../api/stats'
import { PrivateTabData } from '../api/privateTabData'

// API
import * as backgroundAPI from '../api/background'
import * as gridAPI from '../api/topSites/grid'
import { InitialData } from '../api/initialData'
import * as bookmarksAPI from '../api/topSites/bookmarks'
import * as dndAPI from '../api/topSites/dnd'
import * as storage from '../storage'

const initialState = storage.load()

function addToDispatchQueue (fn: Function): void {
  window.setTimeout(fn, 0)
}

export const newTabReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State | undefined, action: any) => {
  console.timeStamp('reducer ' + action.type)
  if (state === undefined) {
    console.timeStamp('reducer init')
    state = initialState
  }

  const startingState = state
  const payload = action.payload
  switch (action.type) {
    case types.NEW_TAB_SET_INITIAL_DATA:
      const initialDataPayload = payload as InitialData
      state = {
        ...state,
        initialDataLoaded: true,
        ...initialDataPayload.preferences,
        stats: initialDataPayload.stats,
        ...initialDataPayload.privateTabData,
        topSites: initialDataPayload.topSites
      }
      if (initialDataPayload.preferences.showBackgroundImage) {
        state.backgroundImage = backgroundAPI.randomBackgroundImage()
      }
      console.timeStamp('reducer initial data received')
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs.
      // See for example the discussion at:
      // https://stackoverflow.com/questions/36730793/can-i-dispatch-an-action-in-reducer
      // This specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
      break

    case types.NEW_TAB_SHOW_SETTINGS_MENU:
      state = { ...state, showSettings: true }
      break
    case types.NEW_TAB_CLOSE_SETTINGS_MENU:
      state = { ...state, showSettings: false }
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
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs. This
      // specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
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
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs. This
      // specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
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
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs. This
      // specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
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
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs. This
      // specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
      break
    }

    case types.NEW_TAB_UNDO_ALL_SITE_IGNORED:
      state = {
        ...state,
        ignoredTopSites: [],
        showSiteRemovalNotification: false
      }
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs. This
      // specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
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
      const stats: Stats = payload.stats
      state = {
        ...state,
        stats
      }
      break

    case types.NEW_TAB_PRIVATE_TAB_DATA_UPDATED:
      const privateTabData = payload as PrivateTabData
      state = {
        ...state,
        useAlternativePrivateSearchEngine: privateTabData.useAlternativePrivateSearchEngine
      }
      break

    case types.NEW_TAB_PREFERENCES_UPDATED:
      const preferences = payload as Preferences
      const shouldChangeBackgroundImage =
        !state.showBackgroundImage && preferences.showBackgroundImage
      state = {
        ...state,
        ...preferences
      }
      if (shouldChangeBackgroundImage) {
        state.backgroundImage = backgroundAPI.randomBackgroundImage()
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
