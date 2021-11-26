// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  TransactionInfo,
  SignMessageRequest,
  SwitchChainRequest,
  TransactionStatus,
  TREZOR_HARDWARE_VENDOR,
  LEDGER_HARDWARE_VENDOR
} from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as PanelActions from '../actions/wallet_panel_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import { TransactionStatusChanged } from '../../common/constants/action_types'
import {
  WalletPanelState,
  PanelState,
  WalletState
} from '../../constants/types'
import {
  AccountPayloadType,
  ShowConnectToSitePayload,
  EthereumChainPayload,
  EthereumChainRequestPayload,
  SignMessagePayload,
  SignMessageProcessedPayload,
  SignMessageHardwareProcessedPayload,
  SwitchEthereumChainProcessedPayload
} from '../constants/action_types'
import {
  findHardwareAccountInfo,
  refreshTransactionHistory
} from '../../common/async/lib'
import {
  signTrezorTransaction,
  signLedgerTransaction,
  signMessageWithHardwareKeyring,
  cancelHardwareOperation,
  dialogErrorFromLedgerErrorCode
} from '../../common/async/hardware'

import { fetchSwapQuoteFactory } from '../../common/async/handlers'
import { Store } from '../../common/async/types'
import { getLocale } from '../../../common/locale'

import getWalletPanelApiProxy from '../wallet_panel_api_proxy'
import { TrezorErrorsCodes } from '../../common/trezor/trezor-messages'
import { LedgerErrorsCodes } from '../../common/ledgerjs/eth_ledger_bridge_keyring'

const handler = new AsyncActionHandler()

function getPanelState (store: Store): PanelState {
  return (store.getState() as WalletPanelState).panel
}

function getWalletState (store: Store): WalletState {
  return store.getState().wallet
}

async function refreshWalletInfo (store: Store) {
  const walletHandler = getWalletPanelApiProxy().walletHandler
  const result = await walletHandler.getWalletInfo()
  store.dispatch(WalletActions.initialized({ ...result, selectedAccount: '', visibleTokens: [] }))
}

async function getPendingChainRequest () {
  const ethJsonRpcController = getWalletPanelApiProxy().ethJsonRpcController
  const chains = (await ethJsonRpcController.getPendingChainRequests()).networks
  if (chains && chains.length) {
    return chains[0]
  }
  return null
}

async function getPendingSwitchChainRequest () {
  const ethJsonRpcController = getWalletPanelApiProxy().ethJsonRpcController
  const requests =
    (await ethJsonRpcController.getPendingSwitchChainRequests()).requests
  if (requests && requests.length) {
    return requests[0]
  }
  return null
}

async function getPendingSignMessageRequest () {
  const braveWalletService = getWalletPanelApiProxy().braveWalletService
  const requests =
    (await braveWalletService.getPendingSignMessageRequests()).requests
  if (requests && requests.length) {
    return requests
  }
  return null
}

handler.on(PanelActions.navigateToMain.getType(), async (store: Store) => {
  const apiProxy = getWalletPanelApiProxy()

  await store.dispatch(PanelActions.navigateTo('main'))
  await store.dispatch(PanelActions.setHardwareWalletInteractionError(undefined))
  apiProxy.panelHandler.setCloseOnDeactivate(true)
  apiProxy.panelHandler.showUI()
})

async function navigateToConnectHardwareWallet (store: Store) {
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.setCloseOnDeactivate(false)

  const { selectedPanel } = getPanelState(store)
  if (selectedPanel === 'connectHardwareWallet') {
    return
  }

  await store.dispatch(PanelActions.navigateTo('connectHardwareWallet'))
  await store.dispatch(PanelActions.setHardwareWalletInteractionError(undefined))
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
    const accounts = url.searchParams.getAll('addr') || []
    const origin = url.searchParams.get('origin') || ''
    store.dispatch(PanelActions.showConnectToSite({ accounts, origin }))
    return
  } else {
    const chain = await getPendingChainRequest()
    if (chain) {
      store.dispatch(PanelActions.addEthereumChain({ chain }))
      return
    }
    const signMessageRequest = await getPendingSignMessageRequest()
    if (signMessageRequest) {
      store.dispatch(PanelActions.signMessage(signMessageRequest))
      return
    }
    const switchChainRequest = await getPendingSwitchChainRequest()
    if (switchChainRequest) {
      store.dispatch(PanelActions.switchEthereumChain(switchChainRequest))
      return
    }
  }
  if (url.hash === '#approveTransaction') {
    // When this panel is explicitly selected we close the panel
    // UI after all transactions are approved or rejected.
    store.dispatch(PanelActions.showApproveTransaction())
    return
  }

  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.cancelConnectToSite.getType(), async (store: Store, payload: AccountPayloadType) => {
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.cancelConnectToSite(payload.siteToConnectTo)
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.cancelConnectHardwareWallet.getType(), async (store: Store, txInfo: TransactionInfo) => {
  const hardwareAccount = await findHardwareAccountInfo(txInfo.fromAddress)
  if (!hardwareAccount || !hardwareAccount.hardware) {
    return
  }
  const apiProxy = getWalletPanelApiProxy()
  await cancelHardwareOperation(apiProxy, hardwareAccount.hardware.vendor)
  // Navigating to main panel view will unmount ConnectHardwareWalletPanel
  // and therefore forfeit connecting to the hardware wallet.
  await store.dispatch(PanelActions.navigateToMain())
})

handler.on(PanelActions.approveHardwareTransaction.getType(), async (store: Store, txInfo: TransactionInfo) => {
  const hardwareAccount = await findHardwareAccountInfo(txInfo.fromAddress)
  if (!hardwareAccount || !hardwareAccount.hardware) {
    return
  }
  await navigateToConnectHardwareWallet(store)
  const apiProxy = getWalletPanelApiProxy()
  if (hardwareAccount.hardware.vendor === LEDGER_HARDWARE_VENDOR) {
    const { success, error, code } = await signLedgerTransaction(apiProxy, hardwareAccount.hardware.path, txInfo)
    if (success) {
      await store.dispatch(PanelActions.navigateToMain())
      refreshTransactionHistory(txInfo.fromAddress)
      return
    }

    if (code) {
      const deviceError = dialogErrorFromLedgerErrorCode(code)
      if (deviceError === 'transactionRejected') {
        await store.dispatch(WalletActions.rejectTransaction(txInfo))
        await store.dispatch(PanelActions.navigateToMain())
        return
      }

      await store.dispatch(PanelActions.setHardwareWalletInteractionError(deviceError))
      return
    }

    if (error) {
      // TODO: handle non-device errors
      console.log(error)
      await store.dispatch(PanelActions.navigateToMain())
    }
  } else if (hardwareAccount.hardware.vendor === TREZOR_HARDWARE_VENDOR) {
    const { success, error, deviceError } = await signTrezorTransaction(apiProxy, hardwareAccount.hardware.path, txInfo)
    if (success) {
      refreshTransactionHistory(txInfo.fromAddress)
      await store.dispatch(PanelActions.navigateToMain())
      return
    }

    if (deviceError === 'deviceBusy') {
      // do nothing as the operation is already in progress
      return
    }

    console.log(error)
    await store.dispatch(WalletActions.rejectTransaction(txInfo))
    await store.dispatch(PanelActions.navigateToMain())
    return
  }

  await store.dispatch(PanelActions.navigateToMain())
})

handler.on(PanelActions.connectToSite.getType(), async (store: Store, payload: AccountPayloadType) => {
  const apiProxy = getWalletPanelApiProxy()
  let accounts: string[] = []
  payload.selectedAccounts.forEach((account) => { accounts.push(account.address) })
  apiProxy.panelHandler.connectToSite(accounts, payload.siteToConnectTo)
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.visibilityChanged.getType(), async (store: Store, isVisible) => {
  if (!isVisible) {
    return
  }
  await refreshWalletInfo(store)
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.showConnectToSite.getType(), async (store: Store, payload: ShowConnectToSitePayload) => {
  store.dispatch(PanelActions.navigateTo('connectWithSite'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.showApproveTransaction.getType(), async (store: Store, payload: ShowConnectToSitePayload) => {
  store.dispatch(PanelActions.navigateTo('approveTransaction'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.addEthereumChain.getType(), async (store: Store, payload: EthereumChainPayload) => {
  store.dispatch(PanelActions.navigateTo('addEthereumChain'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.addEthereumChainRequestCompleted.getType(), async (store: any, payload: EthereumChainRequestPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const ethJsonRpcController = apiProxy.ethJsonRpcController
  ethJsonRpcController.addEthereumChainRequestCompleted(payload.chainId, payload.approved)
  const chain = await getPendingChainRequest()
  if (chain) {
    store.dispatch(PanelActions.addEthereumChain({ chain }))
    return
  }
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.switchEthereumChain.getType(), async (store: Store, request: SwitchChainRequest) => {
  // We need to get current network list first because switch chain doesn't
  // require permission connect first.
  await refreshWalletInfo(store)
  store.dispatch(PanelActions.navigateTo('switchEthereumChain'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.switchEthereumChainProcessed.getType(), async (store: Store, payload: SwitchEthereumChainProcessedPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const ethJsonRpcController = apiProxy.ethJsonRpcController
  ethJsonRpcController.notifySwitchChainRequestProcessed(payload.approved, payload.origin)
  const switchChainRequest = await getPendingSwitchChainRequest()
  if (switchChainRequest) {
    store.dispatch(PanelActions.switchEthereumChain(switchChainRequest))
    return
  }
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.signMessage.getType(), async (store: Store, payload: SignMessagePayload[]) => {
  store.dispatch(PanelActions.navigateTo('signData'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.signMessageProcessed.getType(), async (store: Store, payload: SignMessageProcessedPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const braveWalletService = apiProxy.braveWalletService
  braveWalletService.notifySignMessageRequestProcessed(payload.approved, payload.id)
  const signMessageRequest = await getPendingSignMessageRequest()
  if (signMessageRequest) {
    store.dispatch(PanelActions.signMessage(signMessageRequest))
    return
  }
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.signMessageHardware.getType(), async (store, messageData: SignMessageRequest) => {
  const apiProxy = getWalletPanelApiProxy()
  const hardwareAccount = await findHardwareAccountInfo(messageData.address)
  if (!hardwareAccount || !hardwareAccount.hardware) {
    const braveWalletService = apiProxy.braveWalletService
    braveWalletService.notifySignMessageHardwareRequestProcessed(false, messageData.id,
      '', getLocale('braveWalletHardwareAccountNotFound'))
    const signMessageRequest = await getPendingSignMessageRequest()
    if (signMessageRequest) {
      store.dispatch(PanelActions.signMessage(signMessageRequest))
      return
    }
    apiProxy.panelHandler.closeUI()
    return
  }
  await navigateToConnectHardwareWallet(store)
  const info = hardwareAccount.hardware
  const signed = await signMessageWithHardwareKeyring(apiProxy, info.vendor, info.path, messageData.message)
  if (!signed.success &&
      (signed.code === TrezorErrorsCodes.CommandInProgress ||
       signed.code === LedgerErrorsCodes.TransportLocked)) {
      // do nothing as the operation is already in progress
      return
  }
  const payload: SignMessageHardwareProcessedPayload =
    signed.success ? { success: signed.success, id: messageData.id, signature: signed.payload }
                   : { success: signed.success, id: messageData.id, error: signed.error }
  store.dispatch(PanelActions.signMessageHardwareProcessed(payload))
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.signMessageHardwareProcessed.getType(), async (store, payload: SignMessageHardwareProcessedPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const braveWalletService = apiProxy.braveWalletService
  braveWalletService.notifySignMessageHardwareRequestProcessed(!!payload.success, payload.id, payload.signature || '', payload.error || '')
  const signMessageRequest = await getPendingSignMessageRequest()
  if (signMessageRequest) {
    store.dispatch(PanelActions.signMessage(signMessageRequest))
    return
  }
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.showApproveTransaction.getType(), async (store) => {
  store.dispatch(PanelActions.navigateTo('approveTransaction'))
})

handler.on(PanelActions.setupWallet.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.expandWallet.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet/crypto' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.openWalletApps.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet/crypto/apps' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.expandRestoreWallet.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet/crypto/restore-wallet' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.expandWalletAccounts.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet/crypto/accounts/add-account' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.openWalletSettings.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(WalletActions.transactionStatusChanged.getType(), async (store: Store, payload: TransactionStatusChanged) => {
  const state = getPanelState(store)
  const walletState = getWalletState(store)
  if (
    [TransactionStatus.Submitted, TransactionStatus.Rejected, TransactionStatus.Approved]
      .includes(payload.txInfo.txStatus)
  ) {
    if (state.selectedPanel === 'approveTransaction' && walletState.pendingTransactions.length === 0) {
      const apiProxy = getWalletPanelApiProxy()
      apiProxy.panelHandler.closeUI()
    }
  }
})

handler.on(
  PanelActions.fetchPanelSwapQuote.getType(),
  fetchSwapQuoteFactory(PanelActions.setPanelSwapQuote, PanelActions.setPanelSwapError)
)

export default handler.middleware
