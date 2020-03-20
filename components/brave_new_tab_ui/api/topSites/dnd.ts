/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as gridAPI from './grid'

export const onDraggedSite = (state: NewTab.State, url: string, destUrl: string) => {
  const gridSitesWithoutPreview = gridAPI.getGridSites(state)
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
  state = { ...state, gridSites: gridAPI.getGridSites(state) }
  return state
}

export const onDragEnd = (state: NewTab.State) => {
  state = { ...state, gridSites: gridAPI.getGridSites(state) }
  return state
}
