/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import getActions from '../getActions'

/**
 * Obtains the URL's bookmark info and calls an action with the result
 */
export const fetchBookmarkInfo = (url: string) => {
  chrome.bookmarks.search(url.replace(/^https?:\/\//, ''),
    (bookmarkTreeNodes) => getActions().bookmarkInfoAvailable(url, bookmarkTreeNodes[0] as NewTab.Bookmark)
  )
}

/**
 * Updates bookmark info for top sites based on their state
 */
export const updateBookmarkInfo = (state: NewTab.State, url: string, bookmarkTreeNode?: NewTab.Bookmark) => {
  const bookmarks = state.bookmarks
  const gridSites = state.gridSites.slice()
  const topSites = state.topSites.slice()
  const pinnedTopSites = state.pinnedTopSites.slice()
  // The default empty object is just to avoid null checks below
  const gridSite: Partial<NewTab.Site> = gridSites.find((s) => s.url === url) || {}
  const topSite: Partial<NewTab.Site> = topSites.find((s) => s.url === url) || {}
  const pinnedTopSite: Partial<NewTab.Site> = pinnedTopSites.find((s) => s.url === url) || {}

  if (bookmarkTreeNode) {
    bookmarks[url] = bookmarkTreeNode
    gridSite.bookmarked = topSite.bookmarked = pinnedTopSite.bookmarked = bookmarkTreeNode
  } else {
    delete bookmarks[url]
    gridSite.bookmarked = topSite.bookmarked = pinnedTopSite.bookmarked = undefined
  }
  state = { ...state, bookmarks, gridSites }

  return state
}
