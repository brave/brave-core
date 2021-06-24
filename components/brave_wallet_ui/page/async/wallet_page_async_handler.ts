// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletPageActions from '../actions/wallet_page_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import { CreateWalletPayloadType, RestoreWalletPayloadType } from '../constants/action_types'
import { WalletAPIHandler } from '../../constants/types'

type Store = MiddlewareAPI<Dispatch<AnyAction>, any>

const handler = new AsyncActionHandler()

async function getAPIProxy () {
  // TODO(petemill): don't lazy import() if this actually makes the time-to-first-data slower!
  const api = await import('../wallet_page_api_proxy.js')
  return api.default.getInstance()
}

async function getWalletHandler (): Promise<WalletAPIHandler> {
  const apiProxy = await getAPIProxy()
  return apiProxy.getWalletHandler()
}

async function refreshWalletInfo (store: Store) {
  const walletHandler = await getWalletHandler()
  const result = await walletHandler.getWalletInfo()
  store.dispatch(WalletActions.initialized(result))
}

handler.on(WalletPageActions.createWallet.getType(), async (store, payload: CreateWalletPayloadType) => {
  const apiProxy = await getAPIProxy()
  const result = await apiProxy.createWallet(payload.password)
  store.dispatch(WalletPageActions.walletCreated({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.restoreWallet.getType(), async (store, payload: RestoreWalletPayloadType) => {
  const apiProxy = await getAPIProxy()
  const result = await apiProxy.restoreWallet(payload.mnemonic, payload.password)
  if (!result.isValidMnemonic) {
    store.dispatch(WalletPageActions.hasMnemonicError(!result.isValidMnemonic))
    return
  }
  await apiProxy.notifyWalletBackupComplete()
  await refreshWalletInfo(store)
})

handler.on(WalletPageActions.showRecoveryPhrase.getType(), async (store, payload: boolean) => {
  const apiProxy = await getAPIProxy()
  const result = await apiProxy.getRecoveryWords()
  store.dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.walletSetupComplete.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletPageActions.walletBackupComplete.getType(), async (store) => {
  const apiProxy = await getAPIProxy()
  await apiProxy.notifyWalletBackupComplete()
  await refreshWalletInfo(store)
})

// TODO(bbondy): Remove - Example usage:
// import { AssetPriceTimeframe } from '../../constants/types'
// var price = await walletHandler.getAssetPrice('bat')
// var priceHistory = await walletHandler.getAssetPriceHistory('basic-attention-token', AssetPriceTimeframe.OneWeek)
//
// import { SwapParams } from '../../constants/types'
// const walletHandler = await getWalletHandler()
// var swap_response = await walletHandler.getPriceQuote({
//   takerAddress: '',
//   sellAmount: '',
//   buyAmount: '1000000000000000000000',
//   buyToken: 'ETH',
//   sellToken: 'DAI',
//   buyTokenPercentageFee: 0,
//   slippagePercentage: 0,
//   feeRecipient: '',
//   gasPrice: ''
// })
// console.log('wallet price quote: ', swap_response)
//  var swap_response2 = await walletHandler.getTransactionPayload({
//   takerAddress: '',
//   sellAmount: '',
//   buyAmount: '1000000000000000000000',
//   buyToken: 'ETH',
//   sellToken: 'DAI',
//   buyTokenPercentageFee: 0,
//   slippagePercentage: 0,
//   feeRecipient: '',
//   gasPrice: ''
// })
// console.log(swap_response2)

export default handler.middleware
