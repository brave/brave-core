/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Dispatch } from 'redux'
import { getSearchEngineProvidersSuccess, getBrowserProfilesSuccess } from './actions/welcome_actions'

export const getSearchEngineProviders = () => {
  return (dispatch: Dispatch) => {
    window.cr.sendWithPromise('getSearchEnginesList')
      .then((response: Welcome.SearchEngineListResponse) => {
        dispatch(getSearchEngineProvidersSuccess(response.defaults))
      })
      .catch((error: any) => {
        console.error('Could not load search providers', error)
      })
  }
}

export const getValidBrowserProfiles = (browserProfiles: Array<Welcome.BrowserProfile>): Array<Welcome.BrowserProfile> => {
  const result = browserProfiles.reduce((filteredProfiles, profile) =>
    (profile.name === 'Safari' || profile.name === 'Bookmarks HTML File')
      ? filteredProfiles
      : [...filteredProfiles, profile]
  , [])
  return result
}

export const getBrowserProfiles = () => {
  return (dispatch: Dispatch) => {
    window.cr.sendWithPromise('initializeImportDialog')
      .then((response: Array<Welcome.BrowserProfile>) => {
        const filteredProfiles = getValidBrowserProfiles(response)
        dispatch(getBrowserProfilesSuccess(filteredProfiles))
      })
  }
}
