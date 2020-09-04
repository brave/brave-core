// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  generateGridSiteProperties,
  generateGridSitePropertiesFromDefaultSuperReferralTopSite,
  isExistingGridSite,
  getTopSitesWhitelist,
  filterFromExcludedSites,
  filterDuplicatedSitesbyIndexOrUrl
} from '../helpers/newTabUtils'

export function gridSitesReducerSetFirstRenderDataFromLegacy (
  state: NewTab.GridSitesState,
  legacyState: NewTab.LegacyState | undefined
) {
  return state
}

export function gridSitesReducerSetDefaultSuperReferralTopSites (
  state: NewTab.GridSitesState,
  defaultSuperReferralTopSites: NewTab.DefaultSuperReferralTopSite[]
): NewTab.GridSitesState {
  const newGridSites: NewTab.Site[] = []
  for (const defaultSuperReferralTopSite of defaultSuperReferralTopSites) {
    newGridSites.push(generateGridSitePropertiesFromDefaultSuperReferralTopSite(
                          defaultSuperReferralTopSite))
  }

  state = gridSitesReducerAddSiteOrSites(state, newGridSites)
  return state
}

export function gridSitesReducerUpdateDefaultSuperReferralTopSites (
  state: NewTab.GridSitesState,
  defaultSuperReferralTopSites: NewTab.DefaultSuperReferralTopSite[]
): NewTab.GridSitesState {
  // If defaultSRTopSite is undefined, this data is used from previous brave version.
  // Then, set this property.
  if (state.gridSites.length === 0 || state.gridSites[0].defaultSRTopSite !== undefined) {
    return state
  }

  // So far, we don't tag whether this item comes from default SR or not.
  // W/o this tag, we can't determine whether we should delete this item when
  // history doesn't have this item. Default top sites from SR should not be
  // deleted unless user deletes it explicitely.
  const updatedGridSites: NewTab.Site[] = state.gridSites
  for (const gridSite of updatedGridSites) {
    // Tagging whether this site comes from SR data or not only once.
    if (defaultSuperReferralTopSites.find(site => site.url === gridSite.url)) {
      gridSite['defaultSRTopSite'] = true
    } else {
      gridSite['defaultSRTopSite'] = false
    }
  }
  state = {
    ...state,
    gridSites: updatedGridSites
  }
  return state
}

export function gridSitesReducerSetFirstRenderData (
  state: NewTab.GridSitesState,
  topSites: chrome.topSites.MostVisitedURL[]
): NewTab.GridSitesState {
  const topSitesWhitelisted = getTopSitesWhitelist(topSites)
  // |state.gridSites| has lastly used sites data for NTP.
  // Delete sites from |state.gridSites| that don't exist in topSites(history).
  // Then, |updatedGridSites| will only store previously used sites that topSites have.
  const updatedGridSites: NewTab.Site[] = state.gridSites.filter((gridSite: NewTab.Site) => {
    // Don't delete top sites came from SR's default top sites even if they
    // are not in history.
    if (gridSite.defaultSRTopSite) {
      return true
    }
    return topSitesWhitelisted.some(site => site.url === gridSite.url)
  })
  state = {
    ...state,
    gridSites: updatedGridSites
  }
  const newGridSites: NewTab.Site[] = []
  for (const [index, topSite] of topSitesWhitelisted.entries()) {
    if (isExistingGridSite(state.gridSites, topSite)) {
      // If topSite from Chromium exists in our gridSites list,
      // skip and iterate over the next item.
      continue
    }
    newGridSites.push(generateGridSiteProperties(index, topSite))
  }

  // If there are removed sites coming from a legacy storage,
  // ensure they get filtered.
  const sitesToAdd = filterFromExcludedSites(newGridSites, state.removedSites)
  state = gridSitesReducerAddSiteOrSites(state, sitesToAdd)

  return state
}

export function gridSitesReducerDataUpdated (
  state: NewTab.GridSitesState,
  sitesData: NewTab.Site[]
): NewTab.GridSitesState {
  state = { ...state, gridSites: sitesData }
  return state
}

export function gridSitesReducerUndoRemoveSite (
  state: NewTab.GridSitesState
): NewTab.GridSitesState {
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
  state: NewTab.GridSitesState
): NewTab.GridSitesState {
  // Get all removed sites, assuming the are unique to gridSites
  const allRemovedSites: NewTab.Site[] = state.removedSites
    .filter((site: NewTab.Site) => !isExistingGridSite(state.gridSites, site))

  // Remove all removed sites from the removed list
  state = { ...state, removedSites: [] }

  // Put them back into grid
  state = gridSitesReducerAddSiteOrSites(state, allRemovedSites)
  return state
}

export function gridSitesReducerAddSiteOrSites (
  state: NewTab.GridSitesState,
  addedSites: NewTab.Site[] | NewTab.Site
): NewTab.GridSitesState {
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

  // We have reports of users having duplicated entries
  // after updating to the new storage. This is a special
  // case that breaks all top sites mechanism. Ensure all
  // entries defined visually in the grid are not duplicated.
  const updatedGridSites =
    filterDuplicatedSitesbyIndexOrUrl(currentGridSitesWithNewItems)

  state = gridSitesReducerDataUpdated(state, updatedGridSites)
  return state
}

export function gridSitesReducerShowSiteRemovedNotification (
  state: NewTab.GridSitesState,
  shouldShow: boolean
): NewTab.GridSitesState {
  state = { ...state, shouldShowSiteRemovedNotification: shouldShow }
  return state
}
