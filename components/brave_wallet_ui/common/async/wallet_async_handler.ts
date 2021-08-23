// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletActions from '../actions/wallet_actions'
import {
  UnlockWalletPayloadType,
  ChainChangedEventPayloadType,
  SetInitialVisibleTokensPayloadType
} from '../constants/action_types'
import {
  AppObjectType,
  APIProxyControllers,
  Network,
  WalletState,
  WalletPanelState,
  AssetPriceTimeframe
} from '../../constants/types'
import { InitialVisibleTokenInfo } from '../../options/initial-visible-token-info'

type Store = MiddlewareAPI<Dispatch<AnyAction>, any>

const handler = new AsyncActionHandler()

async function getAPIProxy (): Promise<APIProxyControllers> {
  let api
  if (window.location.hostname === 'wallet-panel.top-chrome') {
    api = await import('../../panel/wallet_panel_api_proxy.js')
  } else {
    api = await import('../../page/wallet_page_api_proxy.js')
  }
  return api.default.getInstance()
}

function getWalletState (store: MiddlewareAPI<Dispatch<AnyAction>, any>): WalletState {
  return (store.getState() as WalletPanelState).wallet
}

async function getTokenPriceHistory (store: Store) {
  const apiProxy = await getAPIProxy()
  const assetPriceController = apiProxy.assetRatioController
  const state = getWalletState(store)
  const result = await Promise.all(state.accounts.map(async (account) => {
    return Promise.all(account.tokens.map(async (token) => {
      return {
        token: token,
        history: await assetPriceController.getPriceHistory(token.asset.symbol.toLowerCase(), state.selectedPortfolioTimeline)
      }
    }))
  }))
  store.dispatch(WalletActions.portfolioPriceHistoryUpdated(result))
}

async function refreshWalletInfo (store: Store) {
  const apiProxy = await getAPIProxy()
  const walletHandler = apiProxy.walletHandler
  const ethJsonRpcController = apiProxy.ethJsonRpcController
  const assetPriceController = apiProxy.assetRatioController
  const result = await walletHandler.getWalletInfo()
  store.dispatch(WalletActions.initialized(result))
  const network = await ethJsonRpcController.getNetwork()
  store.dispatch(WalletActions.setNetwork(network.network))

  // VisibleTokens need to be setup and returned from prefs
  // that away we can map over the contract id's and get the token info for
  // each visibleToken on initialization.
  // In prefs we need to return a different list based on chainID
  const visibleTokensPayload = ['0x0D8775F648430679A709E98d2b0Cb6250d2887EF']
  const visibleTokensInfo = await Promise.all(visibleTokensPayload.map(async (i) => {
    const ercTokenRegistry = (await getAPIProxy()).ercTokenRegistry
    const info = await ercTokenRegistry.getTokenByContract(i)
    return info.token
  }))
  if (visibleTokensInfo[0]) {
    store.dispatch(WalletActions.setVisibleTokensInfo(visibleTokensInfo))
  } else {
    store.dispatch(WalletActions.setVisibleTokensInfo(InitialVisibleTokenInfo))
  }

  // Update ETH Balances
  const state = getWalletState(store)
  const getEthPrice = await assetPriceController.getPrice(['eth'], ['usd'], state.selectedPortfolioTimeline)
  const ethPrice = getEthPrice.success ? getEthPrice.values.find((i) => i.toAsset === 'usd')?.price ?? '0' : '0'
  const getBalanceReturnInfos = await Promise.all(state.accounts.map(async (account) => {
    const balanceInfo = await ethJsonRpcController.getBalance(account.address)
    return balanceInfo
  }))
  const balancesAndPrice = {
    usdPrice: ethPrice,
    balances: getBalanceReturnInfos
  }
  store.dispatch(WalletActions.ethBalancesUpdated(balancesAndPrice))

  // Update Token Balances
  const tokenInfos = state.userVisibleTokensInfo
  const tokenSymbols = tokenInfos.map((token) => {
    return token.symbol.toLowerCase()
  })
  const getTokenPrices = await assetPriceController.getPrice(tokenSymbols, ['usd'], state.selectedPortfolioTimeline)
  const getERCTokenBalanceReturnInfos = await Promise.all(state.accounts.map(async (account) => {
    return Promise.all(tokenInfos.map(async (token) => {
      return ethJsonRpcController.getERC20TokenBalance(token.contractAddress, account.address)
    }))
  }))
  const tokenBalancesAndPrices = {
    balances: getERCTokenBalanceReturnInfos,
    prices: getTokenPrices
  }
  store.dispatch(WalletActions.tokenBalancesUpdated(tokenBalancesAndPrices))
  await getTokenPriceHistory(store)
}

handler.on(WalletActions.initialize.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.chainChangedEvent.getType(), async (store, payload: ChainChangedEventPayloadType) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringCreated.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringRestored.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.locked.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.unlocked.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.backedUp.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.accountsChanged.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.lockWallet.getType(), async (store) => {
  const keyringController = (await getAPIProxy()).keyringController
  await keyringController.lock()
})

handler.on(WalletActions.unlockWallet.getType(), async (store, payload: UnlockWalletPayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  const result = await keyringController.unlock(payload.password)
  store.dispatch(WalletActions.hasIncorrectPassword(!result.success))
})

handler.on(WalletActions.addFavoriteApp.getType(), async (store, appItem: AppObjectType) => {
  const walletHandler = (await getAPIProxy()).walletHandler
  await walletHandler.addFavoriteApp(appItem)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.removeFavoriteApp.getType(), async (store, appItem: AppObjectType) => {
  const walletHandler = (await getAPIProxy()).walletHandler
  await walletHandler.removeFavoriteApp(appItem)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.setInitialVisibleTokens.getType(), async (store, payload: SetInitialVisibleTokensPayloadType) => {
  // We need a walletHandler method 'setInitialVisibleTokens' to use here
  // to set InitialVisibleTokens list
  // const walletHandler = (await getAPIProxy()).walletHandler
  // await walletHandler.setInitialVisibleTokens(payload.visibleAssets)
})

handler.on(WalletActions.selectNetwork.getType(), async (store, payload: Network) => {
  const ethJsonRpcController = (await getAPIProxy()).ethJsonRpcController
  await ethJsonRpcController.setNetwork(payload)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.getAllTokensList.getType(), async (store) => {
  const ercTokenRegistry = (await getAPIProxy()).ercTokenRegistry
  const fullList = await ercTokenRegistry.getAllTokens()
  store.dispatch(WalletActions.setAllTokensList(fullList))
})

handler.on(WalletActions.updateVisibleTokens.getType(), async (store, payload: string[]) => {
  // We need a walletHandler method 'updateVisibleTokens' to use here
  // to update the users visibleTokens
  store.dispatch(WalletActions.setVisibleTokens(payload))
  await refreshWalletInfo(store)
})

handler.on(WalletActions.selectPortfolioTimeline.getType(), async (store, payload: AssetPriceTimeframe) => {
  store.dispatch(WalletActions.portfolioTimelineUpdated(payload))
  await getTokenPriceHistory(store)
})

export default handler.middleware

// TOOD(bbondy): Remove after we have this hooked up
// This is for Ganache seedwords of: garage erosion rapid salmon make wine dragon great away drift jewel evoke
// Don't use this seed with actual funds!
//   const apiProxy = await getAPIProxy()
//  const txData = apiProxy.makeTxData('0x1', '0x14', '0x5208', '0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f', '0xde0b6b3a7640000')
//  const addResult = await apiProxy.ethTxController.addUnapprovedTransaction(txData, '0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7')
//  console.log('addResult: ', addResult)
//  const approveResult = await apiProxy.ethTxController.approveTransaction(addResult.txMetaId)
//  console.log('approveResult: ', approveResult)
//
