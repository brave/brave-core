// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import getWalletPageApiProxy from '../wallet_page_api_proxy'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletPageActions from '../actions/wallet_page_actions'
import { BraveWallet } from '../../constants/types'
import { UpdateSelectedAssetType } from '../constants/action_types'
import { Store } from '../../common/async/types'

const handler = new AsyncActionHandler()

handler.on(
  WalletPageActions.selectAsset.type,
  async (store: Store, payload: UpdateSelectedAssetType) => {
    store.dispatch(WalletPageActions.updateSelectedAsset(payload.asset))
    if (payload.asset) {
      store.dispatch(WalletPageActions.selectPriceTimeframe(payload.timeFrame))
    }
  }
)

handler.on(
  WalletPageActions.addHardwareAccounts.type,
  async (store: Store, accounts: BraveWallet.HardwareWalletAccount[]) => {
    const keyringService = getWalletPageApiProxy().keyringService
    keyringService.addHardwareAccounts(accounts)
  }
)

handler.on(WalletPageActions.openWalletSettings.type, async (store) => {
  chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

export default handler.middleware
