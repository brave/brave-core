// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { ListenerEffectAPI } from '@reduxjs/toolkit'

import * as PanelActions from '../actions/wallet_panel_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import {
  BraveWallet,
  WalletPanelState,
  PanelState,
} from '../../constants/types'
import { cancelHardwareOperation } from '../../common/async/hardware'
import { startAppListening } from '../../common/async/listenerMiddleware'
import type { State, Dispatch } from '../../common/async/types'

import getWalletPanelApiProxy from '../wallet_panel_api_proxy'
import { storeCurrentAndPreviousPanel } from '../../utils/local-storage-utils'

type ListenerApi = ListenerEffectAPI<State, Dispatch>

function getPanelState(listenerApi: ListenerApi): PanelState {
  return (listenerApi.getState() as WalletPanelState).panel
}

async function refreshWalletInfo(listenerApi: ListenerApi) {
  const proxy = getWalletPanelApiProxy()
  const { walletInfo } = await proxy.walletHandler.getWalletInfo()
  const { allAccounts } = await proxy.keyringService.getAllAccounts()
  listenerApi.dispatch(WalletActions.initialized({ walletInfo, allAccounts }))
  listenerApi.dispatch(WalletActions.refreshAll())
}

startAppListening({
  actionCreator: PanelActions.navigateToMain,
  effect: async (_, listenerApi) => {
    const apiProxy = getWalletPanelApiProxy()

    await listenerApi.dispatch(PanelActions.navigateTo('main'))
    await listenerApi.dispatch(
      PanelActions.setHardwareWalletInteractionError(undefined),
    )
    apiProxy.panelHandler.showUI()

    // persist navigation state
    const selectedPanel = (listenerApi.getState() as WalletPanelState).panel
      ?.selectedPanel
    storeCurrentAndPreviousPanel('main', selectedPanel)
  },
})

startAppListening({
  actionCreator: PanelActions.navigateTo,
  effect: async (action, listenerApi) => {
    // navigating away from the current panel, store the last known location
    storeCurrentAndPreviousPanel(
      action.payload,
      (listenerApi.getState() as WalletPanelState).panel?.selectedPanel,
    )
  },
})

startAppListening({
  actionCreator: PanelActions.cancelConnectHardwareWallet,
  effect: async (action, listenerApi) => {
    const payload = action.payload
    if (payload.hardware) {
      await cancelHardwareOperation(
        payload.hardware.vendor,
        payload.accountId.coin,
      )
    }
    // Navigating to main panel view will unmount ConnectHardwareWalletPanel
    // and therefore forfeit connecting to the hardware wallet.
    await listenerApi.dispatch(PanelActions.navigateToMain())
  },
})

startAppListening({
  actionCreator: PanelActions.visibilityChanged,
  effect: async (action, listenerApi) => {
    if (!action.payload) {
      return
    }
    await refreshWalletInfo(listenerApi)
    const apiProxy = getWalletPanelApiProxy()
    apiProxy.panelHandler.showUI()
  },
})

startAppListening({
  actionCreator: PanelActions.showConnectToSite,
  effect: async (_, listenerApi) => {
    listenerApi.dispatch(PanelActions.navigateTo('connectWithSite'))
    const apiProxy = getWalletPanelApiProxy()
    apiProxy.panelHandler.showUI()
  },
})

// Cross-slice action handlers
startAppListening({
  actionCreator: WalletActions.initialize,
  effect: async (_, listenerApi) => {
    const state = getPanelState(listenerApi)
    // Sanity check we only initialize once
    if (state.hasInitialized) {
      return
    }
    // Setup external events
    document.addEventListener('visibilitychange', () => {
      listenerApi.dispatch(
        PanelActions.visibilityChanged(document.visibilityState === 'visible'),
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
        eTldPlusOne: eTldPlusOne,
      }

      listenerApi.dispatch(
        PanelActions.showConnectToSite({ accounts, originInfo }),
      )
      return
    }

    if (url.hash === '#approveTransaction') {
      // When this panel is explicitly selected we close the panel
      // UI after all transactions are approved or rejected.
      listenerApi.dispatch(PanelActions.navigateTo('approveTransaction'))
      getWalletPanelApiProxy().panelHandler.showUI()
      return
    }

    const apiProxy = getWalletPanelApiProxy()
    apiProxy.panelHandler.showUI()
  },
})
