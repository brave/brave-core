/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/rewards_panel_types'

export const createWallet = () => action(types.CREATE_WALLET, {})

export const onWalletCreated = () => action(types.ON_WALLET_CREATED, {})

export const onWalletCreateFailed = () => action(types.ON_WALLET_CREATE_FAILED, {})

export const onTabId = (tabId: number | undefined) => action(types.ON_TAB_ID, {
  tabId
})

export const onTabRetrieved = (tab: chrome.tabs.Tab) => action(types.ON_TAB_RETRIEVED, {
  tab
})

export const onPublisherData = (windowId: number, publisher: RewardsExtension.Publisher) => action(types.ON_PUBLISHER_DATA, {
  windowId,
  publisher
})
