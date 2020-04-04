// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Utils
import { debounce } from '../../common/debounce'
import { keyName as newTabKeyName } from './new_tab_storage'
import { generateGridSitesFromLegacyEntries } from '../helpers/newTabUtils'
const keyName = 'grid-sites-data-v1'
const defaultSuperReferralTopSitesKeyName = 'default-super-referral-top-sites'

const newTabData: any = window.localStorage.getItem(newTabKeyName)
const parsedNewTabData = JSON.parse(newTabData)

const getNewTabData = () => {
  if (parsedNewTabData == null) {
    return {
      pinnedTopSites: [],
      ignoredTopSites: []
    }
  }
  return parsedNewTabData
}

export const initialGridSitesState: NewTab.GridSitesState = {
  gridSites: [],
  removedSites: [],
  shouldShowSiteRemovedNotification: false,
  legacy: {
    // Store legacy pinnedTopSites so users
    // migrating to this new storage won't lose
    // data. Once this change hits the release channel
    // we are safe to remove this bridge
    pinnedTopSites: generateGridSitesFromLegacyEntries(
      getNewTabData().pinnedTopSites
    ),
    ignoredTopSites: generateGridSitesFromLegacyEntries(
      getNewTabData().ignoredTopSites
    )
  }
}

export const load = (): NewTab.GridSitesState => {
  const data: string | null = window.localStorage.getItem(keyName)
  let state = initialGridSitesState
  let storedState: NewTab.GridSitesState

  if (data) {
    try {
      storedState = JSON.parse(data)
      // add defaults for non-peristant data
      state = {
        ...state,
        ...storedState
      }
    } catch (e) {
      console.error('[GridSitesData] Could not parse local storage data: ', e)
    }
  }
  return state
}

export const debouncedSave = debounce<NewTab.GridSitesState>((data: NewTab.GridSitesState) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(data))
  }
}, 50)

export const isDefaultSuperReferralTopSitesAddedToPinnedSites = (): boolean => {
  return window.localStorage.getItem(defaultSuperReferralTopSitesKeyName) !== null
}

export const setDefaultSuperReferralTopSitesAddedToPinnedSites = () => {
  window.localStorage.setItem(defaultSuperReferralTopSitesKeyName, 'set')
}
