/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'
import { Dispatch } from 'redux'

// Constants
import { types } from '../constants/welcome_types'

export const importNowRequested = () => action(types.IMPORT_NOW_REQUESTED)

export const goToTabRequested = (url: string, target: string) => action(types.GO_TO_TAB_REQUESTED, {
  url,
  target
})

export const closeTabRequested = () => action(types.CLOSE_TAB_REQUESTED)

export const changeDefaultSearchProvider = (searchProvider: string) => action(types.CHANGE_DEFAULT_SEARCH_PROVIDER, searchProvider)

const getSearchEngineProvidersStarted = () => action(types.IMPORT_DEFAULT_SEARCH_PROVIDERS_STARTED)

const getSearchEngineProvidersSuccess = (searchProviders: Array<Welcome.SearchEngineEntry>) => action(types.IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS, searchProviders)

const getSearchEngineProvidersFailure = () => action(types.IMPORT_DEFAULT_SEARCH_PROVIDERS_FAILURE)

export const getSearchEngineProviders = () => {
  return (dispatch: Dispatch) => {
    dispatch(getSearchEngineProvidersStarted())

    // @ts-ignore
    window.cr.sendWithPromise('getSearchEnginesList')
      .then((response: Welcome.SearchEngineListResponse) => {
        dispatch(getSearchEngineProvidersSuccess(response.defaults))
      })
      .catch(() => {
        dispatch(getSearchEngineProvidersFailure())
      })
  }
}
