// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletActions from '../actions/wallet_actions'
import { UnlockWalletPayloadType } from '../constants/action_types'
import { WalletAPIHandler, AppObjectType } from '../../constants/types'

type Store = MiddlewareAPI<Dispatch<AnyAction>, any>

const handler = new AsyncActionHandler()

async function getWalletHandler (): Promise<WalletAPIHandler> {
  let api
  if (window.location.hostname === 'wallet-panel.top-chrome') {
    api = await import('../../panel/wallet_panel_api_proxy.js')
  } else {
    api = await import('../../page/wallet_page_api_proxy.js')
  }
  return api.default.getInstance().getWalletHandler()
}

async function refreshWalletInfo (store: Store) {
  const walletHandler = await getWalletHandler()
  const result = await walletHandler.getWalletInfo()
  store.dispatch(WalletActions.initialized(result))
}

handler.on(WalletActions.initialize.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.lockWallet.getType(), async (store) => {
  const walletHandler = await getWalletHandler()
  await walletHandler.lockWallet()
  await refreshWalletInfo(store)
})

handler.on(WalletActions.unlockWallet.getType(), async (store, payload: UnlockWalletPayloadType) => {
  const walletHandler = await getWalletHandler()
  await walletHandler.unlockWallet(payload.password)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.addFavoriteApp.getType(), async (store, appItem: AppObjectType) => {
  const walletHandler = await getWalletHandler()
  await walletHandler.addFavoriteApp(appItem)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.removeFavoriteApp.getType(), async (store, appItem: AppObjectType) => {
  const walletHandler = await getWalletHandler()
  await walletHandler.removeFavoriteApp(appItem)
  await refreshWalletInfo(store)
})

export default handler.middleware
