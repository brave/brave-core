/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { Reducer } from 'redux'

// Constants
import { loadTimeData } from '../../common/loadTimeData'
import { types } from '../constants/welcome_types'

// Utils
import * as storage from '../storage'

const welcomeReducer: Reducer<Welcome.State | undefined> = (state: Welcome.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  const payload = action.payload
  const startingState = state
  switch (action.type) {
    case types.IMPORT_BROWSER_DATA_REQUESTED:
      const importTypes = {
        import_dialog_autofill_form_data: true,
        import_dialog_bookmarks: true,
        import_dialog_history: true,
        import_dialog_saved_passwords: true,
        import_dialog_search_engine: true,
        import_dialog_extensions: true,
        import_dialog_payments: true
      }
      chrome.send('importData', [payload, importTypes])
      break
    case types.GO_TO_TAB_REQUESTED:
      window.open(payload.url, payload.target)
      break
    case types.CLOSE_TAB_REQUESTED:
      window.close()
      break
    case types.CHANGE_DEFAULT_SEARCH_PROVIDER:
      const modelIndex = parseInt(payload, 10)
      chrome.send('setDefaultSearchEngine', [modelIndex])
      break
    case types.IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS:
      // TODO(bsclifton): remove when ready for other regions
      const showBraveSearch: boolean =
          ['US', 'CA'].includes(loadTimeData.getString('countryString'))
      // Only show Brave Search during onboarding for US/CA
      const filteredSearchList = payload.filter((item: any) => {
        return !(item.name && item.name.startsWith('Brave ') && item.canBeRemoved)
      })
      state = {
        ...state,
        searchProviders: showBraveSearch ? payload : filteredSearchList
      }
      break
    case types.IMPORT_BROWSER_PROFILES_SUCCESS:
      state = { ...state, browserProfiles: payload }
      break
    case types.RECORD_P3A:
      let details = payload.details || {}
      chrome.send('recordP3A', [
        details.currentScreen,
        details.finished,
        details.skipped
      ])
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default welcomeReducer
