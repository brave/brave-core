// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  generateGridSiteProperties,
  isExistingGridSite,
  getGridSitesWhitelist,
  isGridSitePinned,
  isGridSiteBookmarked
} from '../helpers/newTabUtils'

export function gridSitesReducerSetFirstRenderData (
  state: NewTab.State,
  topSites: chrome.topSites.MostVisitedURL[]
): NewTab.State {
  const gridSitesWhitelist = getGridSitesWhitelist(topSites)
  const newGridSites: NewTab.Site[] = []
  for (const [index, topSite] of gridSitesWhitelist.entries()) {
    if (isExistingGridSite(state.gridSites, topSite)) {
      // If topSite from Chromium exists in our gridSites list,
      // skip and iterate over the next item.
      continue
    }
    newGridSites.push(generateGridSiteProperties(index, topSite))
  }
  state = gridSitesReducerAddSiteOrSites(state, newGridSites)
  return state
}

export function gridSitesReducerDataUpdated (
  state: NewTab.State,
  sitesData: NewTab.Site[]
): NewTab.State {
  let updatedGridSites: NewTab.Site[] = []
  let isolatedPinnedSites: NewTab.Site[] = []

  // Separate pinned sites from un-pinned sites. This step is needed
  // since the list length is unknown, so pinned items need the updated
  // list to be full before looking for its index.
  // See test "preserve pinnedIndex positions after random reordering"
  for (const site of sitesData) {
    if (site.pinnedIndex !== undefined) {
      isolatedPinnedSites.push(site)
    } else {
      updatedGridSites.push(site)
    }
  }
  // Get the pinned site and add it to the index specified by pinnedIndex.
  // all items after it will be pushed one index up.
  for (const pinnedSite of isolatedPinnedSites) {
    if (pinnedSite.pinnedIndex !== undefined) {
      updatedGridSites.splice(pinnedSite.pinnedIndex, 0, pinnedSite)
    }
  }

  state = { ...state, gridSites: updatedGridSites }
  return state
}

export function gridSitesReducerToggleSitePinned (
  state: NewTab.State,
  pinnedSite: NewTab.Site
): NewTab.State {
  const updatedGridSites: NewTab.Site[] = []
  for (const [index, gridSite] of state.gridSites.entries()) {
    if (gridSite.url === pinnedSite.url) {
      updatedGridSites.push({
        ...gridSite,
        pinnedIndex: isGridSitePinned(gridSite) ? undefined : index
      })
    } else {
      updatedGridSites.push(gridSite)
    }
  }
  state = gridSitesReducerDataUpdated(state, updatedGridSites)
  return state
}

export function gridSitesReducerRemoveSite (
  state: NewTab.State,
  removedSite: NewTab.Site
): NewTab.State {
  state = {
    ...state,
    removedSites: [ ...state.removedSites, removedSite ]
  }

  const filterRemovedFromGridSites = state.gridSites
    .filter((site: NewTab.Site) => {
      // In updatedGridSites we only want sites not removed by the user
      return state.removedSites
        .every((removedSite: NewTab.Site) => removedSite.url !== site.url)
    })
  state = gridSitesReducerDataUpdated(state, filterRemovedFromGridSites)
  return state
}

export function gridSitesReducerUndoRemoveSite (
  state: NewTab.State
): NewTab.State {
  if (state.removedSites.length < 0) {
    return state
  }

  // Remove and modify removed list
  const removedItem: NewTab.Site | undefined = state.removedSites.pop()

  if (
    removedItem === undefined ||
    isExistingGridSite(state.gridSites, removedItem)
  ) {
    return state
  }

  // Push item back into the grid list by adding the site
  state = gridSitesReducerAddSiteOrSites(state, removedItem)
  return state
}

export function gridSitesReducerUndoRemoveAllSites (
  state: NewTab.State
): NewTab.State {
  // Get all removed sites, assuming the are unique to gridSites
  const allRemovedSites: NewTab.Site[] = state.removedSites
    .filter((site: NewTab.Site) => !isExistingGridSite(state.gridSites, site))

  // Remove all removed sites from the removed list
  state = { ...state, removedSites: [] }

  // Put them back into grid
  state = gridSitesReducerAddSiteOrSites(state, allRemovedSites)
  return state
}

export const gridSitesReducerUpdateSiteBookmarkInfo = (
  state: NewTab.State,
  bookmarkInfo: chrome.bookmarks.BookmarkTreeNode
): NewTab.State => {
  const updatedGridSites: NewTab.Site[] = []
  for (const [index, gridSite] of state.gridSites.entries()) {
    const updatedBookmarkTreeNode = bookmarkInfo[index]
    if (
      updatedBookmarkTreeNode !== undefined &&
      gridSite.url === updatedBookmarkTreeNode.url
    ) {
      updatedGridSites.push({
        ...gridSite,
        bookmarkInfo: updatedBookmarkTreeNode
      })
    } else {
      updatedGridSites.push(gridSite)
    }
  }
  state = gridSitesReducerDataUpdated(state, updatedGridSites)
  return state
}

export const gridSitesReducerToggleSiteBookmarkInfo = (
  state: NewTab.State,
  url: string,
  bookmarkInfo: chrome.bookmarks.BookmarkTreeNode
): NewTab.State => {
  const updatedGridSites: NewTab.Site[] = []
  for (const gridSite of state.gridSites) {
    if (url === gridSite.url) {
      updatedGridSites.push({
        ...gridSite,
        bookmarkInfo: isGridSiteBookmarked(bookmarkInfo)
          ? undefined
          // Add a transitory state for bookmarks.
          // This will be overriden by a new mount and is used
          // as a secondary render until data is ready,
          : { title: gridSite.title, id: 'TEMPORARY' }
      })
    } else {
      updatedGridSites.push(gridSite)
    }
  }
  state = gridSitesReducerDataUpdated(state, updatedGridSites)
  return state
}

export function gridSitesReducerAddSiteOrSites (
  state: NewTab.State,
  addedSites: NewTab.Site[] | NewTab.Site
): NewTab.State {
  const sitesToAdd: NewTab.Site[] = Array.isArray(addedSites)
    ? addedSites
    : [addedSites]

  if (sitesToAdd.length === 0) {
    return state
  }
  const currentGridSitesWithNewItems: NewTab.Site[] = [
    // The order here is important: ensure recently added items
    // come first so users can see it when grid is full. This is
    // also useful to undo a site removal, which would re-populate
    // the grid in the first positions.
    ...sitesToAdd,
    ...state.gridSites
  ]
  state = gridSitesReducerDataUpdated(state, currentGridSitesWithNewItems)
  return state
}

export function gridSitesReducerShowSiteRemovedNotification (
  state: NewTab.State,
  shouldShow: boolean
): NewTab.State {
  state = { ...state, shouldShowSiteRemovedNotification: shouldShow }
  return state
}
