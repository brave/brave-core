// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as Actions from '../actions/wallet_page_actions'
import { PageState, WalletPageState } from '../../constants/types'
import { CreateWalletPayloadType } from '../constants/action_types'

const handler = new AsyncActionHandler()

async function getAPIProxy () {
  // TODO(petemill): don't lazy import() if this actually makes the time-to-first-data slower!
  const api = await import('../../wallet_panel_api_proxy.js')
  return api.default.getInstance()
}

function getPageState (store: MiddlewareAPI<Dispatch<AnyAction>, any>): PageState {
  return (store.getState() as WalletPageState).page
}

handler.on(Actions.initialize.getType(), async (store) => {
  const state = getPageState(store)
  // Sanity check we only initialize once
  if (state.hasInitialized) {
    return
  }
  // TODO: Fetch any data we need for initial display, instead of fake wait.
  await new Promise(resolve => setTimeout(resolve, 400))
  store.dispatch(Actions.initialized({ isConnected: true }))
  return
})

handler.on(Actions.createWallet.getType(), async (store, payload: CreateWalletPayloadType) => {
  const apiProxy = await getAPIProxy()
  const mnemonic = await apiProxy.createWallet(payload.password);
  console.log('createWallet!!!!!', payload, mnemonic);

  store.dispatch(Actions.walletCreated({ mnemonic }))
})

export default handler.middleware
