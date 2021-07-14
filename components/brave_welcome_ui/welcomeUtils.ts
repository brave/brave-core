/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Dispatch } from 'redux'
import { sendWithPromise } from '../common/cr'
import {
  getSearchEngineProvidersSuccess,
  getBrowserProfilesSuccess
} from './actions/welcome_actions'
import { State as ImportBoxState } from './containers/screens/importBox'

// Search box

export const getSearchEngineProviders = () => {
  return (dispatch: Dispatch) => {
    sendWithPromise('getSearchEnginesList')
      .then((response: Welcome.SearchEngineListResponse) => {
        dispatch(getSearchEngineProvidersSuccess(response.defaults))
      })
      .catch((error: any) => {
        console.error('Could not load search providers', error)
      })
  }
}

// Import Box

export const getValidBrowserProfiles = (browserProfiles: Array<Welcome.BrowserProfile>): Array<Welcome.BrowserProfile> => {
  const result = browserProfiles.reduce((filteredProfiles, profile) =>
    (profile.name === 'Safari' || profile.name === 'Bookmarks HTML File')
      ? filteredProfiles
      : [...filteredProfiles, profile]
  , [])
  return result
}

export const getBrowserProfiles = () =>
  async (dispatch: Dispatch) => {
    const response = await sendWithPromise<Array<Welcome.BrowserProfile>>('initializeImportDialog')
    const filteredProfiles = getValidBrowserProfiles(response)
    dispatch(getBrowserProfilesSuccess(filteredProfiles))
  }

export const getSelectedBrowserProfile = (profileIndex: string, browserProfiles: Array<Welcome.BrowserProfile>) => {
  return browserProfiles.find((profile: Welcome.BrowserProfile) =>
    profile.index.toString() === profileIndex
  )
}

export const getSourceBrowserProfileIndex = (state: ImportBoxState): number => {
  return state && state.selectedBrowserProfile && state.selectedBrowserProfile.index || 0
}

export const isValidBrowserProfiles = (browserProfiles: Array<Welcome.BrowserProfile>) =>
  browserProfiles && Array.isArray(browserProfiles) && browserProfiles.length > 0
