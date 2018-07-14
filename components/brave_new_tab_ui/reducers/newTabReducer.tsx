/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

const types = require('../constants/newTabTypes')
const storage = require('../storage')
let getGridSites_

const getGridSites = (...args) => {
  if (!getGridSites_) {
    getGridSites_ = require('../api').getGridSites
  }
  return getGridSites_(...args)
}

const updateBookmarkInfo = (state, url, bookmarkTreeNode) => {
  const bookmarks = Object.assign({}, state.bookmarks)
  const gridSites = state.gridSites.slice()
  const topSites = state.topSites.slice()
  const pinnedTopSites = state.pinnedTopSites.slice()
  // The default empty object is just to avoid null checks below
  const gridSite = gridSites.find((s) => s.url === url) || {}
  const topSite = topSites.find((s) => s.url === url) || {}
  const pinnedTopSite = pinnedTopSites.find((s) => s.url === url) || {}
  gridSite.bookmarked = topSite.bookmarked = pinnedTopSite.bookmarked = !!bookmarkTreeNode
  if (bookmarkTreeNode) {
    bookmarks[url] = bookmarkTreeNode
  } else {
    delete bookmarks[url]
  }
  state = { ...state, bookmarks, gridSites }

  return state
}

const onDraggedSite = (state, url, destUrl, dragRight) => {
  const gridSitesWithoutPreview = getGridSites(state, false)
  const currentPositionIndex = gridSitesWithoutPreview.findIndex(site => site.url === url)
  const finalPositionIndex = gridSitesWithoutPreview.findIndex(site => site.url === destUrl)
  let pinnedTopSites = state.pinnedTopSites.slice()

  // A site that is not pinned yet will become pinned
  const pinnedMovingSite = pinnedTopSites.find(site => site.url === url)
  if (!pinnedMovingSite) {
    const movingTopSite = Object.assign({}, gridSitesWithoutPreview.find(site => site.url === url))
    movingTopSite.index = currentPositionIndex
    movingTopSite.pinned = true
    pinnedTopSites.push(movingTopSite)
  }

  pinnedTopSites = pinnedTopSites.map((pinnedTopSite) => {
    pinnedTopSite = Object.assign({}, pinnedTopSite)
    const currentIndex = pinnedTopSite.index
    if (currentIndex === currentPositionIndex) {
      pinnedTopSite.index = finalPositionIndex
    } else if (currentIndex > currentPositionIndex && pinnedTopSite.index <= finalPositionIndex) {
      pinnedTopSite.index = pinnedTopSite.index - 1
    } else if (currentIndex < currentPositionIndex && pinnedTopSite.index >= finalPositionIndex) {
      pinnedTopSite.index = pinnedTopSite.index + 1
    }
    return pinnedTopSite
  })
  state = { ...state, pinnedTopSites }
  state = { ...state, gridSites: getGridSites(state, false) }
  return state
}

const onDragEnd = (state) => {
  state = { ...state, gridSites: getGridSites(state, false) }
  return state
}

let calculateGridSites
const newTabReducer = (state, action) => {
  if (state === undefined) {
    calculateGridSites = require('../api').calculateGridSites
    setImmediate(() => {
      const { fetchTopSites } = require('../api')
      fetchTopSites()
    })
    state = storage.load() || {}
    state = Object.assign(storage.getInitialState(), state)
  }

  const startingState = state
  switch (action.type) {
    case types.BOOKMARK_ADDED:
      const { fetchBookmarkInfo } = require('../api')
      const topSite = state.topSites.findIndex((site) => site.url === action.url)
      chrome.bookmarks.create({
        title: topSite.title,
        url: topSite.url
      }, () => {
        fetchBookmarkInfo(action.url)
      })
      break
    case types.BOOKMARK_REMOVED:
      const bookmarkInfo = state.bookmarks[action.url]
      if (bookmarkInfo) {
        chrome.bookmarks.remove(bookmarkInfo.id, () => {
          fetchBookmarkInfo(action.url)
        })
      }
      break
    case types.NEW_TAB_TOP_SITES_DATA_UPDATED:
      state = { ...state, topSites: action.topSites }
      calculateGridSites(state)
      break
    case types.NEW_TAB_BACKGROUND_IMAGE_LOAD_FAILED: {
      const source = '/50cc52a4f1743ea74a21da996fe44272.jpg'
      const fallbackImage = {
        name: 'Bay Bridge',
        source,
        style: { backgroundImage: 'url(' + source + ')' },
        author: 'Darrell Sano',
        link: 'https://dksfoto.smugmug.com'
      }
      if (!state.imageLoadFailed) {
        state = {
          ...state,
          backgroundImage: fallbackImage,
          imageLoadFailed: true
        }
      } else {
        state = {
          ...state,
          backgroundImage: undefined,
          showImages: false
        }
      }
      break
    }

    case types.NEW_TAB_SITE_PINNED: {
      const topSiteIndex = state.topSites.findIndex((site) => site.url === action.url)
      const pinnedTopSite = Object.assign({}, state.topSites[topSiteIndex], { pinned: true })
      const pinnedTopSites = state.pinnedTopSites.slice()
      pinnedTopSite.index = topSiteIndex
      pinnedTopSites.push(pinnedTopSite)
      pinnedTopSites.sort((x, y) => x.index - y.index)
      state = {
        ...state,
        pinnedTopSites
      }
      calculateGridSites(state)
      break
    }

    case types.NEW_TAB_SITE_UNPINNED:
      const currentPositionIndex = state.pinnedTopSites.findIndex((site) => site.url === action.url)
      if (currentPositionIndex !== -1) {
        const pinnedTopSites = state.pinnedTopSites.slice()
        pinnedTopSites.splice(currentPositionIndex, 1)
        state = {
          ...state,
          pinnedTopSites
        }
      }
      calculateGridSites(state)
      break

    case types.NEW_TAB_SITE_IGNORED: {
      const topSiteIndex = state.topSites.findIndex((site) => site.url === action.url)
      const ignoredTopSites = state.ignoredTopSites.slice()
      ignoredTopSites.splice(0, 1, state.topSites[topSiteIndex])
      state = {
        ...state,
        ignoredTopSites,
        showSiteRemovalNotification: true
      }
      calculateGridSites(state)
      break
    }

    case types.NEW_TAB_UNDO_SITE_IGNORED: {
      const ignoredTopSites = state.ignoredTopSites.slice()
      ignoredTopSites.pop()
      state = {
        ...state,
        ignoredTopSites,
        showSiteRemovalNotification: false
      }
      calculateGridSites(state)
      break
    }

    case types.NEW_TAB_UNDO_ALL_SITE_IGNORED:
      state = {
        ...state,
        ignoredTopSites: [],
        showSiteRemovalNotification: false
      }
      calculateGridSites(state)
      break

    case types.NEW_TAB_HIDE_SITE_REMOVAL_NOTIFICATION:
      state = {
        ...state,
        showSiteRemovalNotification: false
      }
      break

    case types.NEW_TAB_SITE_DRAGGED:
      state = onDraggedSite(state, action.fromUrl, action.toUrl, action.dragRight)
      break

    case types.NEW_TAB_SITE_DRAG_END:
      state = onDragEnd(state, action.url)
      break

    case types.NEW_TAB_BOOKMARK_INFO_AVAILABLE:
      state = updateBookmarkInfo(state, action.queryUrl, action.bookmarkTreeNode)
      break

    case types.NEW_TAB_GRID_SITES_UPDATED:
      state = { ...state, gridSites: action.gridSites }
      break

    case types.NEW_TAB_STATS_UPDATED:
      state = storage.getLoadTimeData(state)
      break

    case types.NEW_TAB_USE_ALTERNATIVE_PRIVATE_SEARCH_ENGINE:
      state = { ...state, useAlternativePrivateSearchEngine: action.shouldUse }
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
