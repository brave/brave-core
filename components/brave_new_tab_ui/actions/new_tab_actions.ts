/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/new_tab_types'

export const topSitesDataUpdated = (topSites: NewTab.Site[]) => action(types.NEW_TAB_TOP_SITES_DATA_UPDATED, {
  topSites
})

export const bookmarkAdded = (url: string) => action(types.BOOKMARK_ADDED, {
  url
})

export const bookmarkRemoved = (url: string) => action(types.BOOKMARK_REMOVED, {
  url
})

export const sitePinned = (url: string) => action(types.NEW_TAB_SITE_PINNED, {
  url
})

export const siteUnpinned = (url: string) => action(types.NEW_TAB_SITE_UNPINNED, {
  url
})

export const siteIgnored = (url: string) => action(types.NEW_TAB_SITE_IGNORED, {
  url
})

export const undoSiteIgnored = (url: string) => action(types.NEW_TAB_UNDO_SITE_IGNORED, {
  url
})

export const undoAllSiteIgnored = (url: string) => action(types.NEW_TAB_UNDO_ALL_SITE_IGNORED, {
  url
})

export const siteDragged = (fromUrl: string, toUrl: string, dragRight: boolean) => action(types.NEW_TAB_SITE_DRAGGED, {
  fromUrl,
  toUrl,
  dragRight
})

export const siteDragEnd = (url: string, didDrop: boolean) => action(types.NEW_TAB_SITE_DRAG_END, {
  url,
  didDrop
})

export const onHideSiteRemovalNotification = () => action(types.NEW_TAB_HIDE_SITE_REMOVAL_NOTIFICATION)

export const bookmarkInfoAvailable = (queryUrl: string, bookmarkTreeNode: NewTab.Bookmark) => action(types.NEW_TAB_BOOKMARK_INFO_AVAILABLE, {
  queryUrl,
  bookmarkTreeNode
})

export const gridSitesUpdated = (gridSites: NewTab.Site[]) => action(types.NEW_TAB_GRID_SITES_UPDATED, {
  gridSites
})

export const statsUpdated = () => action(types.NEW_TAB_STATS_UPDATED)

export const changePrivateSearchEngine = (shouldUse: boolean) => action(types.NEW_TAB_USE_ALTERNATIVE_PRIVATE_SEARCH_ENGINE, {
  shouldUse
})

export const showSettingsMenu = () => action(types.NEW_TAB_SHOW_SETTINGS_MENU)

export const closeSettingsMenu = () => action(types.NEW_TAB_CLOSE_SETTINGS_MENU)

export const preferencesUpdated = (prefs: any) => action(types.NEW_TAB_PREFERENCES_UPDATED, {
  prefs
})
