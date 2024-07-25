// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'
import { Store } from 'redux'

// actions
import { PanelActions } from '../common/slices/panel.slice'

// types
import type WalletApiProxy from '../common/wallet_api_proxy'
import { BraveWallet, SupportedCoinTypes } from '../constants/types'

export function handleEndpointError(
  endpointName: string,
  friendlyMessage: string,
  error: any
) {
  const message = `${friendlyMessage}: ${error?.message || error}`
  console.log(`error in: ${endpointName || 'endpoint'}: ${message}`)
  console.error(error)
  return {
    error: friendlyMessage
  }
}

export async function getEnabledCoinTypes(api: WalletApiProxy) {
  const { isBitcoinEnabled, isZCashEnabled } = (
    await api.walletHandler.getWalletInfo()
  ).walletInfo

  // Get All Networks
  return SupportedCoinTypes.filter((coin) => {
    return (
      coin === BraveWallet.CoinType.FIL ||
      coin === BraveWallet.CoinType.SOL ||
      (coin === BraveWallet.CoinType.BTC && isBitcoinEnabled) ||
      (coin === BraveWallet.CoinType.ZEC && isZCashEnabled) ||
      coin === BraveWallet.CoinType.ETH
    )
  })
}

export async function getVisibleNetworksList(api: WalletApiProxy) {
  const { jsonRpcService } = api

  const enabledCoinTypes = await getEnabledCoinTypes(api)
  const { networks: allNetworks } = await jsonRpcService.getAllNetworks()

  const networks = (
    await mapLimit(enabledCoinTypes, 10, async (coin: number) => {
      const { chainIds: hiddenChainIds } =
        await jsonRpcService.getHiddenNetworks(coin)
      return allNetworks.filter((n) => !hiddenChainIds.includes(n.chainId))
    })
  ).flat(1)

  return networks
}

export function navigateToConnectHardwareWallet(
  panelHandler: BraveWallet.PanelHandlerRemote,
  store: Pick<Store, 'dispatch' | 'getState'>
) {
  panelHandler.setCloseOnDeactivate(false)

  const selectedPanel: string | undefined =
    store.getState()?.panel?.selectedPanel

  if (selectedPanel === 'connectHardwareWallet') {
    return
  }

  store.dispatch(PanelActions.navigateTo('connectHardwareWallet'))
  store.dispatch(PanelActions.setHardwareWalletInteractionError(undefined))
}
