// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../common/AsyncActionHandler'
import * as Actions from '../actions/wallet_panel_actions'
import { State, WalletPanelReducerState } from '../constants/types'
import { AccountPayloadType } from '../constants/action_types'

const handler = new AsyncActionHandler()

async function getAPIProxy() {
  // TODO(petemill): don't lazy import() if this actually makes the time-to-first-data slower!
  const api = await import('../wallet_panel_api_proxy.js')
  return api.default.getInstance()
}

function getState (store: MiddlewareAPI<Dispatch<AnyAction>, any>): WalletPanelReducerState {
  return (store.getState() as State).walletPanelReducer
}

handler.on(Actions.initialize.getType(), async (store) => {
  const state = getState(store)
  // Sanity check we only initialize once
  if (state.hasInitialized) {
    return
  }
  // Setup external events
  document.addEventListener('visibilitychange', () => {
    store.dispatch(Actions.visibilityChanged(document.visibilityState === 'visible'))
  })
  const apiProxy = await getAPIProxy()
  apiProxy.showUI()
  // TODO: Fetch any data we need for initial display, instead of fake wait.
  await new Promise(resolve => setTimeout(resolve, 400))
  store.dispatch(Actions.initialized({isConnected: true}))
  return
})

handler.on(Actions.cancelConnectToSite.getType(), async (store) => {
  const apiProxy = await getAPIProxy()
  console.log('cancel connect to site!')
  apiProxy.closeUI()
})

handler.on(Actions.connectToSite.getType(), async (store, payload: AccountPayloadType) => {
  const apiProxy = await getAPIProxy()
  console.log('Got connect to site for accounts: ', payload.selectedAccounts, 'for site:', payload.siteToConnectTo)
  apiProxy.closeUI()
})

handler.on(Actions.visibilityChanged.getType(), async (store, isVisible) => {
  if (!isVisible) {
    return
  }
  const apiProxy = await getAPIProxy()
  apiProxy.showUI()
})

export default handler.middleware
