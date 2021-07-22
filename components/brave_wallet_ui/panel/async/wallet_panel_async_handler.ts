// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as PanelActions from '../actions/wallet_panel_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import { WalletPanelState, PanelState, WalletAPIHandler } from '../../constants/types'
import { AccountPayloadType } from '../constants/action_types'

type Store = MiddlewareAPI<Dispatch<AnyAction>, any>

const handler = new AsyncActionHandler()

async function getAPIProxy () {
  // TODO(petemill): don't lazy import() if this actually makes the time-to-first-data slower!
  const api = await import('../wallet_panel_api_proxy.js')
  return api.default.getInstance()
}

function getPanelState (store: MiddlewareAPI<Dispatch<AnyAction>, any>): PanelState {
  return (store.getState() as WalletPanelState).panel
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

handler.on(WalletActions.initialize.getType(), async (store) => {
  const state = getPanelState(store)
  // Sanity check we only initialize once
  if (state.hasInitialized) {
    return
  }
  // Setup external events
  document.addEventListener('visibilitychange', () => {
    store.dispatch(PanelActions.visibilityChanged(document.visibilityState === 'visible'))
  })
  const apiProxy = await getAPIProxy()
  apiProxy.showUI()
})

handler.on(PanelActions.cancelConnectToSite.getType(), async (store) => {
  const apiProxy = await getAPIProxy()
  apiProxy.closeUI()
})

handler.on(PanelActions.connectToSite.getType(), async (store, payload: AccountPayloadType) => {
  const apiProxy = await getAPIProxy()
  apiProxy.closeUI()
})

handler.on(PanelActions.visibilityChanged.getType(), async (store, isVisible) => {
  if (!isVisible) {
    return
  }
  await refreshWalletInfo(store)
  const apiProxy = await getAPIProxy()
  apiProxy.showUI()
})

handler.on(PanelActions.setupWallet.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet' })
})

handler.on(PanelActions.expandWallet.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet' })
})

handler.on(PanelActions.openWalletApps.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet#apps' })
})

handler.on(PanelActions.restoreWallet.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet#restore' })
})

export default handler.middleware
