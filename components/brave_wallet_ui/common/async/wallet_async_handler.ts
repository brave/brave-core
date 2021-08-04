// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletActions from '../actions/wallet_actions'
import {
  UnlockWalletPayloadType,
  SetInitialAccountNamesPayloadType,
  AddNewAccountNamePayloadType,
  ChainChangedEventPayloadType,
  SetInitialVisibleTokensPayloadType
} from '../constants/action_types'
import {
  AppObjectType,
  APIProxyControllers,
  Network
} from '../../constants/types'

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

async function refreshWalletInfo (store: Store) {
  const walletHandler = (await getAPIProxy()).walletHandler
  const result = await walletHandler.getWalletInfo()
  store.dispatch(WalletActions.initialized(result))
  const ethJsonRpcController = (await getAPIProxy()).ethJsonRpcController
  const network = await ethJsonRpcController.getNetwork()
  store.dispatch(WalletActions.setNetwork(network.network))
}

handler.on(WalletActions.initialize.getType(), async (store) => {
  // VisibleTokens need to be setup and returned from prefs
  // that away we can map over the contract id's and get the token info for
  // each visibleToken on initialization.
  // In prefs we need to return a different list based on chainID
  const walletHandler = (await getAPIProxy()).walletHandler
  const visibleTokensPayload = ['0x0D8775F648430679A709E98d2b0Cb6250d2887EF']
  const visibleTokensInfo = await Promise.all(visibleTokensPayload.map(async (i) => {
    const info = await walletHandler.getTokenByContract(i)
    return info.token
  }))
  store.dispatch(WalletActions.setVisibleTokensInfo(visibleTokensInfo))
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
  const walletHandler = (await getAPIProxy()).walletHandler
  await walletHandler.setInitialVisibleAssets(payload.visibleAssets)
})

handler.on(WalletActions.setInitialAccountNames.getType(), async (store, payload: SetInitialAccountNamesPayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  await keyringController.setInitialAccountNames(payload.accountNames)
})

handler.on(WalletActions.addNewAccountName.getType(), async (store, payload: AddNewAccountNamePayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  await keyringController.addNewAccountName(payload.accountName)
})

handler.on(WalletActions.selectNetwork.getType(), async (store, payload: Network) => {
  const ethJsonRpcController = (await getAPIProxy()).ethJsonRpcController
  await ethJsonRpcController.setNetwork(payload)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.getAllTokensList.getType(), async (store) => {
  const walletHandler = (await getAPIProxy()).walletHandler
  const fullList = await walletHandler.getAllTokens()
  store.dispatch(WalletActions.setAllTokensList(fullList))
})

handler.on(WalletActions.updateVisibleTokens.getType(), async (store, payload: string[]) => {
  // We need a walletHandler method 'updateVisibleTokens' to use here
  // to update the users visibleTokens
  store.dispatch(WalletActions.setVisibleTokens(payload))
  await refreshWalletInfo(store)
})

export default handler.middleware
