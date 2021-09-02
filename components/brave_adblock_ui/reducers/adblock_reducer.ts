/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/adblock_types'

// Utils
import * as storage from '../storage'
import { debounce } from '../../common/debounce'

const updateCustomFilters = debounce((customFilters: string) => {
  chrome.send('brave_adblock.updateCustomFilters', [customFilters])
}, 1500)

const adblockReducer: Reducer<AdBlock.State | undefined> = (state: AdBlock.State | undefined, action) => {
  if (state === undefined) {
    state = storage.load()
  }

  const startingState = state
  switch (action.type) {
    case types.ADBLOCK_ENABLE_FILTER_LIST:
      chrome.send('brave_adblock.enableFilterList', [action.payload.uuid, action.payload.enabled])
      state = {
        ...state,
        settings: {
          ...state.settings,
          regionalLists: state.settings.regionalLists.map(resource =>
            resource.uuid === action.payload.uuid ? { ...resource, enabled: action.payload.enabled } : resource
          )
        }
      }
      break
    case types.ADBLOCK_GET_CUSTOM_FILTERS:
      chrome.send('brave_adblock.getCustomFilters')
      break
    case types.ADBLOCK_GET_REGIONAL_LISTS:
      chrome.send('brave_adblock.getRegionalLists')
      break
    case types.ADBLOCK_GET_LIST_SUBSCRIPTIONS:
      chrome.send('brave_adblock.getListSubscriptions')
      break
    case types.ADBLOCK_SUBMIT_NEW_SUBSCRIPTION:
      chrome.send('brave_adblock.submitNewSubscription', [action.payload.listUrl])
      break
    case types.ADBLOCK_SET_SUBSCRIPTION_ENABLED:
      chrome.send('brave_adblock.setSubscriptionEnabled', [action.payload.listUrl, action.payload.enabled])
      break
    case types.ADBLOCK_DELETE_SUBSCRIPTION:
      chrome.send('brave_adblock.deleteSubscription', [action.payload.listUrl])
      break
    case types.ADBLOCK_REFRESH_SUBSCRIPTION:
      chrome.send('brave_adblock.refreshSubscription', [action.payload.listUrl])
      break
    case types.ADBLOCK_VIEW_SUBSCRIPTION_SOURCE:
      chrome.send('brave_adblock.viewSubscriptionSource', [action.payload.listUrl])
      break
    case types.ADBLOCK_ON_GET_CUSTOM_FILTERS:
      state = { ...state, settings: { ...state.settings, customFilters: action.payload.customFilters } }
      break
    case types.ADBLOCK_ON_GET_REGIONAL_LISTS:
      state = { ...state, settings: { ...state.settings, regionalLists: action.payload.regionalLists } }
      break
    case types.ADBLOCK_ON_GET_LIST_SUBSCRIPTIONS:
      state = { ...state, settings: { ...state.settings, listSubscriptions: action.payload.listSubscriptions } }
      break
    case types.ADBLOCK_UPDATE_CUSTOM_FILTERS:
      state = { ...state, settings: { ...state.settings, customFilters: action.payload.customFilters } }
      updateCustomFilters(state.settings.customFilters)
      break
    default:
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default adblockReducer
