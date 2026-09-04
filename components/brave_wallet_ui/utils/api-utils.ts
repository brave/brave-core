// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { Store } from 'redux'

// actions
import { PanelActions } from '../panel/actions'

// utils
import getAPIProxy from '../common/async/bridge'

export function handleEndpointError(
  endpointName: string,
  friendlyMessage: string,
  error: any,
) {
  const message = `${friendlyMessage}: ${error?.message || error}`
  console.log(`error in: ${endpointName || 'endpoint'}: ${message}`)
  console.error(error)
  return {
    error: friendlyMessage,
  }
}

export function navigateToConnectHardwareWallet(
  store: Pick<Store, 'dispatch' | 'getState'>,
) {
  const selectedPanel: string | undefined =
    store.getState()?.panel?.selectedPanel

  if (selectedPanel === 'connectHardwareWallet') {
    return
  }

  store.dispatch(PanelActions.navigateTo('connectHardwareWallet'))
  store.dispatch(
    PanelActions.setHardwareWalletInteractionError('deviceNotConnected'),
  )
}

export const getHasPendingRequests = async () => {
  const { braveWalletService, jsonRpcService, txService } = getAPIProxy()

  const { count: pendingTxsCount } =
    await txService.getPendingTransactionsCount()
  if (pendingTxsCount > 0) {
    return true
  }

  const { requests: signSolTxsRequests } =
    await braveWalletService.getPendingSignSolTransactionsRequests()
  if (signSolTxsRequests.length) {
    return true
  }

  const { requests: signCardanoTxRequests } =
    await braveWalletService.getPendingSignCardanoTransactionRequests()
  if (signCardanoTxRequests.length) {
    return true
  }

  const { requests: signMessageRequests } =
    await braveWalletService.getPendingSignMessageRequests()
  if (signMessageRequests.length) {
    return true
  }

  const { requests: addTokenRequests } =
    await braveWalletService.getPendingAddSuggestTokenRequests()
  if (addTokenRequests.length) {
    return true
  }

  const { requests: decryptRequests } =
    await braveWalletService.getPendingDecryptRequests()
  if (decryptRequests.length) {
    return true
  }

  const { requests: publicKeyRequests } =
    await braveWalletService.getPendingGetEncryptionPublicKeyRequests()
  if (publicKeyRequests.length) {
    return true
  }

  const { requests: addChainRequests } =
    await jsonRpcService.getPendingAddChainRequests()
  if (addChainRequests.length) {
    return true
  }

  const { requests: switchChainRequests } =
    await jsonRpcService.getPendingSwitchChainRequests()
  if (switchChainRequests.length) {
    return true
  }

  return false
}
