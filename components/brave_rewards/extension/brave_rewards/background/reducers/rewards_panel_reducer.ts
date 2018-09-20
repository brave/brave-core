import { RewardsPanelState } from '../../constants/rewardsPanelState'

import { types } from '../../constants/rewards_panel_types'
import { getTabData } from '../api/tabs_api'

const defaultState: RewardsPanelState = {
  walletCreated: false,
  walletCreateFailed: false,
  publisher: undefined
}

export const rewardsPanelReducer = (state: RewardsPanelState = defaultState, action: any) => {
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
      // call get publisher info for specific url
      break
  }

  return state
}
