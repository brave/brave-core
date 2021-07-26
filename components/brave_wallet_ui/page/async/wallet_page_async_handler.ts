// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletPageActions from '../actions/wallet_page_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import { CreateWalletPayloadType, RestoreWalletPayloadType, UpdateSelectedAssetType, AddAccountToWalletPayloadType } from '../constants/action_types'
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
  store.dispatch(WalletActions.setInitialAccountNames({ accountNames: ['Account 1'] }))
  store.dispatch(WalletPageActions.walletCreated({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.restoreWallet.getType(), async (store, payload: RestoreWalletPayloadType) => {
  const apiProxy = await getAPIProxy()
  const result = await apiProxy.restoreWallet(payload.mnemonic, payload.password)
  if (!result.isValidMnemonic) {
    store.dispatch(WalletPageActions.hasMnemonicError(!result.isValidMnemonic))
    return
  }
  store.dispatch(WalletActions.setInitialAccountNames({ accountNames: ['Account 1'] }))
  await apiProxy.notifyWalletBackupComplete()
  await refreshWalletInfo(store)
})

handler.on(WalletPageActions.addAccountToWallet.getType(), async (store, payload: AddAccountToWalletPayloadType) => {
  const apiProxy = await getAPIProxy()
  const result = await apiProxy.addAccountToWallet()
  store.dispatch(WalletActions.addNewAccountName({ accountName: payload.accountName }))
  await refreshWalletInfo(store)
  return result.success
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

handler.on(WalletPageActions.selectAsset.getType(), async (store, payload: UpdateSelectedAssetType) => {
  store.dispatch(WalletPageActions.updateSelectedAsset(payload.asset))
  store.dispatch(WalletPageActions.setIsFetchingPriceHistory(true))
  const walletHandler = await getWalletHandler()
  if (payload.asset) {
    const priceInfo = await walletHandler.getAssetPrice([payload.asset.symbol.toLowerCase()], ['usd', 'btc'])
    const priceHistory = await walletHandler.getAssetPriceHistory(payload.asset.symbol.toLowerCase(), payload.timeFrame)
    store.dispatch(WalletPageActions.updatePriceInfo({ priceHistory: priceHistory, usdPriceInfo: priceInfo.values[0], btcPriceInfo: priceInfo.values[1], timeFrame: payload.timeFrame }))
  } else {
    store.dispatch(WalletPageActions.updatePriceInfo({ priceHistory: undefined, btcPriceInfo: undefined, usdPriceInfo: undefined, timeFrame: payload.timeFrame }))
  }
})

// TODO(bbondy): Remove - Example usage:
//
// Swap API
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
//
// Interacting with the token registry
// const val1 = await walletHandler.getAllTokens()
// const val2 = await walletHandler.getTokenBySymbol('BAT')
// const val3 = await walletHandler.getTokenByContract('0x0D8775F648430679A709E98d2b0Cb6250d2887EF')
// const val4 = await walletHandler.getTokenByContract('dsaida')
// console.log('val1-4: ', val1, val2, val3, val4)
//
// Getting and setting network:
//
// import { Network } from '../../constants/types'
// const network = await walletHandler.getNetwork()
// await walletHandler.setNetwork(Network.Rinkeby)
// const chainId = await walletHandler.getChainId()
// const blockTrackerUrl = await walletHandler.getBlockTrackerUrl()
//
// Getting ETH and BAT ERC20 balance
// const balance = await walletHandler.getBalance(address)
// const token_balance = await walletHandler.getERC20TokenBalance('0x0d8775f648430679a709e98d2b0cb6250d2887ef', address)

export default handler.middleware
