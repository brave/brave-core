import { RewardsPanelState } from '../../constants/rewardsPanelState'

import { types } from '../../constants/rewards_panel_types'
import * as storage from '../storage'
import { getTabData } from '../api/tabs_api'

export const rewardsPanelReducer = (state: RewardsPanelState | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  const startingState = state

  const payload = action.payload
  switch (action.type) {
    case types.CREATE_WALLET:
      chrome.braveRewards.createWallet()
      break
    case types.ON_WALLET_CREATED:
      state = { ...state }
      state.walletCreated = true
      break
    case types.ON_WALLET_CREATE_FAILED:
      state = { ...state }
      state.walletCreateFailed = true
      break
    case types.ON_TAB_ID:
      if (payload.tabId) {
        getTabData(payload.tabId)
      }
      break
    case types.ON_TAB_RETRIEVED:
      // TODO add caching for url's, so that we know
      // which url in connected to which publisher key
      if (!payload.tab) {
        break
      }
      state = { ...state }
      console.log(payload.tab)
      // state.publisher = undefined
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}
