/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// API
import getActions from '../getActions'
import * as bookmarksAPI from './bookmarks'
import { getCharForSite } from '../../helpers/newTabUtils'

// Utils
import { debounce } from '../../../common/debounce'

export const getGridSites = (state: NewTab.State, checkBookmarkInfo?: boolean) => {
  const sizeToCount = { large: 18, medium: 12, small: 6 }
  const count = sizeToCount[state.gridLayoutSize || 'small']
  const defaultChromeWebStoreUrl = 'https://chrome.google.com/webstore'

  // Start with top sites with filtered out ignored sites and pinned sites
  let gridSites = state.topSites.slice()
  .filter((site) =>
    !state.ignoredTopSites.find((ignoredSite) => ignoredSite.url === site.url) &&
    !state.pinnedTopSites.find((pinnedSite) => pinnedSite.url === site.url) &&
    // see https://github.com/brave/brave-browser/issues/5376
    !site.url.startsWith(defaultChromeWebStoreUrl)
  )

  // Then add in pinned sites at the specified index, these need to be added in the same
  // order as the index they are.
  const pinnedTopSites = state.pinnedTopSites
    .slice()
    .sort((x, y) => x.index - y.index)
  pinnedTopSites.forEach((pinnedSite) => {
    gridSites.splice(pinnedSite.index, 0, pinnedSite)
  })

  gridSites = gridSites.slice(0, count)
  gridSites.forEach((gridSite: NewTab.Site) => {
    gridSite.letter = getCharForSite(gridSite)
    gridSite.thumb = `chrome://thumb/${gridSite.url}`
    gridSite.favicon = `chrome://favicon/size/64@1x/${gridSite.url}`
    gridSite.bookmarked = state.bookmarks[gridSite.url]

    if (checkBookmarkInfo && !gridSite.bookmarked) {
      bookmarksAPI.fetchBookmarkInfo(gridSite.url)
    }
  })
  return gridSites
}

/**
 * Calculates the top sites grid and calls an action with the results
 */
export const calculateGridSites = debounce((state: NewTab.State) => {
  // TODO(petemill):
  // Instead of debouncing at the point of reducing actions to state,
  // and having the reducer call this, it may be more understandable
  // (and performant) to have this be a selector so that the calculation
  // is only performed when the relevant state data is changed.
  getActions().gridSitesUpdated(getGridSites(state, true))
}, 10)
