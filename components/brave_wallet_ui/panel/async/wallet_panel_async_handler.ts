// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as PanelActions from '../actions/wallet_panel_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import {
  BraveWallet,
  WalletPanelState,
  PanelState,
  PanelTypes,
  HardwareVendor
} from '../../constants/types'
import { ShowConnectToSitePayload } from '../constants/action_types'
import { cancelHardwareOperation } from '../../common/async/hardware'

import { Store } from '../../common/async/types'
import getWalletPanelApiProxy from '../wallet_panel_api_proxy'
import { storeCurrentAndPreviousPanel } from '../../utils/local-storage-utils'

const handler = new AsyncActionHandler()

function getPanelState(store: Store): PanelState {
  return (store.getState() as WalletPanelState).panel
}

async function refreshWalletInfo(store: Store) {
  const proxy = getWalletPanelApiProxy()
  const { walletInfo } = await proxy.walletHandler.getWalletInfo()
  const { allAccounts } = await proxy.keyringService.getAllAccounts()
  store.dispatch(WalletActions.initialized({ walletInfo, allAccounts }))
  store.dispatch(WalletActions.refreshAll())
}

handler.on(PanelActions.navigateToMain.type, async (store: Store) => {
  const apiProxy = getWalletPanelApiProxy()

  await store.dispatch(PanelActions.navigateTo('main'))
  await store.dispatch(
    PanelActions.setHardwareWalletInteractionError(undefined)
  )
  apiProxy.panelHandler.setCloseOnDeactivate(true)
  apiProxy.panelHandler.showUI()

  // persist navigation state
  const selectedPanel = store.getState().panel?.selectedPanel
  storeCurrentAndPreviousPanel('main', selectedPanel)
})

handler.on(
  PanelActions.navigateTo.type,
  async (store: Store, payload: PanelTypes) => {
    // navigating away from the current panel, store the last known location
    storeCurrentAndPreviousPanel(payload, store.getState().panel?.selectedPanel)
  }
)

handler.on(
  PanelActions.cancelConnectHardwareWallet.type,
  async (store: Store, payload: BraveWallet.AccountInfo) => {
    if (payload.hardware) {
      // eslint-disable-next-line max-len
      // eslint-disable @typescript-eslint/no-unnecessary-type-assertion
      await cancelHardwareOperation(
        payload.hardware.vendor as HardwareVendor,
        payload.accountId.coin
      )
    }
    // Navigating to main panel view will unmount ConnectHardwareWalletPanel
    // and therefore forfeit connecting to the hardware wallet.
    await store.dispatch(PanelActions.navigateToMain())
  }
)

handler.on(
  PanelActions.visibilityChanged.type,
  async (store: Store, isVisible) => {
    if (!isVisible) {
      return
    }
    await refreshWalletInfo(store)
    const apiProxy = getWalletPanelApiProxy()
    apiProxy.panelHandler.showUI()
  }
)

handler.on(
  PanelActions.showConnectToSite.type,
  async (store: Store, payload: ShowConnectToSitePayload) => {
    store.dispatch(PanelActions.navigateTo('connectWithSite'))
    const apiProxy = getWalletPanelApiProxy()
    apiProxy.panelHandler.showUI()
  }
)

handler.on(
  PanelActions.setCloseOnDeactivate.type,
  async (store: Store, payload: boolean) => {
    getWalletPanelApiProxy().panelHandler.setCloseOnDeactivate(payload)
  }
)

// Cross-slice action handlers
handler.on(WalletActions.initialize.type, async (store) => {
  const state = getPanelState(store)
  // Sanity check we only initialize once
  if (state.hasInitialized) {
    return
  }
  // Setup external events
  document.addEventListener('visibilitychange', () => {
    store.dispatch(
      PanelActions.visibilityChanged(document.visibilityState === 'visible')
    )
  })

  // Parse webUI URL, dispatch showConnectToSite action if needed.
  // TODO(jocelyn): Extract ConnectToSite UI pieces out from panel UI.
  const url = new URL(window.location.href)
  if (url.hash === '#connectWithSite') {
    const accounts = url.searchParams.getAll('addr') || []
    const originSpec = url.searchParams.get('origin-spec') || ''
    const eTldPlusOne = url.searchParams.get('etld-plus-one') || ''
    const originInfo: BraveWallet.OriginInfo = {
      originSpec: originSpec,
      eTldPlusOne: eTldPlusOne
    }

    store.dispatch(PanelActions.showConnectToSite({ accounts, originInfo }))
    return
  }

  if (url.hash === '#approveTransaction') {
    // When this panel is explicitly selected we close the panel
    // UI after all transactions are approved or rejected.
    store.dispatch(PanelActions.navigateTo('approveTransaction'))
    getWalletPanelApiProxy().panelHandler.showUI()
    return
  }

  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

export default handler.middleware
