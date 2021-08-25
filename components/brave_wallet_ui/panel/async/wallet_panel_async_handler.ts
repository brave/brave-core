// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as PanelActions from '../actions/wallet_panel_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import { WalletPanelState, PanelState } from '../../constants/types'
import { AccountPayloadType, ShowConnectToSitePayload } from '../constants/action_types'

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

async function refreshWalletInfo (store: Store) {
  const walletHandler = (await getAPIProxy()).walletHandler
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

  // Parse webUI URL, dispatch showConnectToSite action if needed.
  // TODO(jocelyn): Extract ConnectToSite UI pieces out from panel UI.
  const url = new URL(window.location.href)

  if (url.hash === '#connectWithSite') {
    const tabId = Number(url.searchParams.get('tabId')) || -1
    const accounts = url.searchParams.getAll('addr') || []
    const origin = url.searchParams.get('origin') || ''
    store.dispatch(PanelActions.showConnectToSite({ tabId, accounts, origin }))
    return
  }

  const apiProxy = await getAPIProxy()
  apiProxy.showUI()
})

handler.on(PanelActions.cancelConnectToSite.getType(), async (store, payload: AccountPayloadType) => {
  const state = getPanelState(store)
  const apiProxy = await getAPIProxy()
  apiProxy.cancelConnectToSite(payload.siteToConnectTo, state.tabId)
  apiProxy.closeUI()
})

handler.on(PanelActions.connectToSite.getType(), async (store, payload: AccountPayloadType) => {
  const state = getPanelState(store)
  const apiProxy = await getAPIProxy()
  let accounts: string[] = []
  payload.selectedAccounts.forEach((account) => { accounts.push(account.address) })
  apiProxy.connectToSite(accounts, payload.siteToConnectTo, state.tabId)
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

handler.on(PanelActions.showConnectToSite.getType(), async (store, payload: ShowConnectToSitePayload) => {
  store.dispatch(PanelActions.navigateTo('connectWithSite'))
  const apiProxy = await getAPIProxy()
  apiProxy.showUI()
})

handler.on(PanelActions.setupWallet.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet' }).catch((e) => { console.error(e) })
})

handler.on(PanelActions.expandWallet.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet' }).catch((e) => { console.error(e) })
})

handler.on(PanelActions.openWalletApps.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet#apps' }).catch((e) => { console.error(e) })
})

handler.on(PanelActions.restoreWallet.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet#restore' }).catch((e) => { console.error(e) })
})

handler.on(PanelActions.openWalletSettings.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://settings/wallet' }).catch((e) => { console.error(e) })
})

export default handler.middleware
