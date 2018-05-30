/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const types = require('../constants/newTabTypes')
export const topSitesDataUpdated = (topSites) => {
  return {
    type: types.NEW_TAB_TOP_SITES_DATA_UPDATED,
    topSites
  }
}

export const backgroundImageLoadFailed = () => ({
  type: types.NEW_TAB_BACKGROUND_IMAGE_LOAD_FAILED
})

export const bookmarkAdded = (url) => ({
  type: types.BOOKMARK_ADDED,
  url
})

export const bookmarkRemoved = (url) => ({
  type: types.BOOKMARK_REMOVED,
  url
})

export const sitePinned = (url) => ({
  type: types.NEW_TAB_SITE_PINNED,
  url
})

export const siteUnpinned = (url) => ({
  type: types.NEW_TAB_SITE_UNPINNED,
  url
})

export const siteIgnored = (url) => ({
  type: types.NEW_TAB_SITE_IGNORED,
  url
})

export const undoSiteIgnored = (url) => ({
  type: types.NEW_TAB_UNDO_SITE_IGNORED,
  url
})

export const undoAllSiteIgnored = (url) => ({
  type: types.NEW_TAB_UNDO_ALL_SITE_IGNORED,
  url
})

export const siteDragged = (fromUrl, toUrl, dragRight) => ({
  type: types.NEW_TAB_SITE_DRAGGED,
  fromUrl,
  toUrl,
  dragRight
})

export const siteDragEnd = (url, didDrop) => ({
  type: types.NEW_TAB_SITE_DRAG_END,
  url,
  didDrop
})

export const onHideSiteRemovalNotification = () => ({
  type: types.NEW_TAB_HIDE_SITE_REMOVAL_NOTIFICATION
})

export const bookmarkInfoAvailable = (queryUrl, bookmarkTreeNode) => ({
  type: types.NEW_TAB_BOOKMARK_INFO_AVAILABLE,
  queryUrl,
  bookmarkTreeNode
})

export const gridSitesUpdated = (gridSites) => ({
  type: types.NEW_TAB_GRID_SITES_UPDATED,
  gridSites
})

export const statsUpdated = () => ({
  type: types.NEW_TAB_STATS_UPDATED
})

export const changePrivateSearchEngine = (shouldUse) => ({
  type: types.NEW_TAB_USE_ALTERNATIVE_PRIVATE_SEARCH_ENGINE,
  shouldUse
})
