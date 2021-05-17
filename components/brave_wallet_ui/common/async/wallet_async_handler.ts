// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletActions from '../actions/wallet_actions'

const handler = new AsyncActionHandler()

async function getWalletHandler() {
  let api
  if (window.location.hostname === 'wallet-panel.top-chrome') {
    api = await import('../../panel/wallet_panel_api_proxy.js')
  } else {
    api = await import('../../page/wallet_page_api_proxy.js')
  }
  return await api.default.getInstance().getWalletHandler()
}


handler.on(WalletActions.initialize.getType(), async (store) => {
  const walletHandler = await getWalletHandler()
  const result = await walletHandler.initialize()
  store.dispatch(WalletActions.initialized({ isWalletCreated: result.isWalletCreated }))
})

export default handler.middleware;
