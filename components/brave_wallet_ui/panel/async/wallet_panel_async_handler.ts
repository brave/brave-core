// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as PanelActions from '../actions/wallet_panel_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import { TransactionStatusChanged } from '../../common/constants/action_types'
import {
  BraveWallet,
  WalletPanelState,
  PanelState,
  WalletState,
  WalletRoutes,
  SerializableTransactionInfo,
  SerializableSignMessageRequest,
  SerializableSwitchChainRequest
} from '../../constants/types'
import {
  ConnectWithSitePayloadType,
  AddSuggestTokenProcessedPayload,
  CancelConnectHardwareWalletPayload,
  ShowConnectToSitePayload,
  EthereumChainRequestPayload,
  SignMessageProcessedPayload,
  SignAllTransactionsProcessedPayload,
  SwitchEthereumChainProcessedPayload,
  GetEncryptionPublicKeyProcessedPayload,
  DecryptProcessedPayload
} from '../constants/action_types'
import {
  findHardwareAccountInfo,
  refreshTransactionHistory
} from '../../common/async/lib'
import {
  signTrezorTransaction,
  signLedgerEthereumTransaction,
  signMessageWithHardwareKeyring,
  signRawTransactionWithHardwareKeyring,
  cancelHardwareOperation,
  dialogErrorFromLedgerErrorCode,
  dialogErrorFromTrezorErrorCode,
  signLedgerFilecoinTransaction,
  signLedgerSolanaTransaction
} from '../../common/async/hardware'

import { Store } from '../../common/async/types'
import { getLocale } from '../../../common/locale'
import getWalletPanelApiProxy from '../wallet_panel_api_proxy'
import { isRemoteImageURL } from '../../utils/string-utils'
import { HardwareVendor } from 'components/brave_wallet_ui/common/api/hardware_keyrings'
import {
  deserializeOrigin,
  makeSerializableDecryptRequest,
  makeSerializableGetEncryptionPublicKeyRequest,
  makeSerializableSignMessageRequest,
  makeSerializableSwitchChainRequest
} from '../../utils/model-serialization-utils'

const handler = new AsyncActionHandler()

function getPanelState (store: Store): PanelState {
  return (store.getState() as WalletPanelState).panel
}

function getWalletState (store: Store): WalletState {
  return store.getState().wallet
}

async function refreshWalletInfo (store: Store) {
  const walletHandler = getWalletPanelApiProxy().walletHandler
  const result = (await walletHandler.getWalletInfo()).walletInfo
  store.dispatch(WalletActions.initialized({ ...result, selectedAccount: '', visibleTokens: [] }))
}

async function hasPendingUnlockRequest () {
  const keyringService = getWalletPanelApiProxy().keyringService
  return (await keyringService.hasPendingUnlockRequest()).pending
}

async function getPendingAddChainRequest () {
  const jsonRpcService = getWalletPanelApiProxy().jsonRpcService
  const requests = (await jsonRpcService.getPendingAddChainRequests()).requests
  if (requests && requests.length) {
    return requests[0]
  }
  return null
}

async function getPendingSwitchChainRequest () {
  const jsonRpcService = getWalletPanelApiProxy().jsonRpcService
  const requests =
    (await jsonRpcService.getPendingSwitchChainRequests()).requests
  if (requests && requests.length) {
    return requests[0]
  }
  return null
}

async function getPendingGetEncryptionPublicKeyRequest () {
  const braveWalletService = getWalletPanelApiProxy().braveWalletService
  const requests =
    (await braveWalletService.getPendingGetEncryptionPublicKeyRequests()).requests
  if (requests && requests.length) {
    return requests[0]
  }
  return null
}

async function getPendingDecryptRequest () {
  const braveWalletService = getWalletPanelApiProxy().braveWalletService
  const requests =
    (await braveWalletService.getPendingDecryptRequests()).requests
  if (requests && requests.length) {
    return requests[0]
  }
  return null
}

async function getPendingSignMessageRequests () {
  const braveWalletService = getWalletPanelApiProxy().braveWalletService
  const requests =
    (await braveWalletService.getPendingSignMessageRequests()).requests
  if (requests && requests.length) {
    return requests
  }
  return null
}

async function getPendingSignTransactionRequests () {
  const { braveWalletService } = getWalletPanelApiProxy()
  const { requests } = await braveWalletService.getPendingSignTransactionRequests()

  if (requests && requests.length) {
    return requests
  }
  return null
}

async function getPendingSignAllTransactionsRequests () {
  const { braveWalletService } = getWalletPanelApiProxy()
  const { requests } = await braveWalletService.getPendingSignAllTransactionsRequests()

  if (requests && requests.length) {
    return requests
  }
  return null
}

async function getPendingAddSuggestTokenRequest () {
  const braveWalletService = getWalletPanelApiProxy().braveWalletService
  const requests =
    (await braveWalletService.getPendingAddSuggestTokenRequests()).requests
  if (requests && requests.length) {
    const logo = requests[0].token.logo
    if (logo !== '' && !isRemoteImageURL(logo)) {
      requests[0].token.logo = `chrome://erc-token-images/${logo}`
    }
    return requests[0]
  }
  return null
}

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

handler.on(PanelActions.navigateToMain.type, async (store: Store) => {
  const apiProxy = getWalletPanelApiProxy()

  await store.dispatch(PanelActions.navigateTo('main'))
  await store.dispatch(PanelActions.setHardwareWalletInteractionError(undefined))
  apiProxy.panelHandler.setCloseOnDeactivate(true)
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.cancelConnectToSite.type, async () => {
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.cancelConnectToSite()
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.cancelConnectHardwareWallet.type, async (store: Store, payload: CancelConnectHardwareWalletPayload) => {
  const found = await findHardwareAccountInfo(payload.accountAddress)
  if (found && found.hardware) {
    const info: BraveWallet.HardwareInfo = found.hardware
    // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
    await cancelHardwareOperation(info.vendor as HardwareVendor, payload.coinType)
  }
  // Navigating to main panel view will unmount ConnectHardwareWalletPanel
  // and therefore forfeit connecting to the hardware wallet.
  await store.dispatch(PanelActions.navigateToMain())
})

handler.on(PanelActions.approveHardwareTransaction.type, async (store: Store, txInfo: SerializableTransactionInfo) => {
  const found = await findHardwareAccountInfo(txInfo.fromAddress)
  if (!found || !found.hardware) {
    return
  }
  const hardwareAccount: BraveWallet.HardwareInfo = found.hardware
  await navigateToConnectHardwareWallet(store)
  const apiProxy = getWalletPanelApiProxy()
  if (hardwareAccount.vendor === BraveWallet.LEDGER_HARDWARE_VENDOR) {
    let success, error, code
    switch (found.coin) {
      case BraveWallet.CoinType.ETH:
        ({ success, error, code } = await signLedgerEthereumTransaction(apiProxy, hardwareAccount.path, txInfo, found.coin))
        break
      case BraveWallet.CoinType.FIL:
        ({ success, error, code } = await signLedgerFilecoinTransaction(apiProxy, txInfo, found.coin))
        break
      case BraveWallet.CoinType.SOL:
        ({ success, error, code } = await signLedgerSolanaTransaction(apiProxy, hardwareAccount.path, txInfo.id, found.coin))
        break
      default:
        await store.dispatch(PanelActions.navigateToMain())
        return
    }
    if (success) {
      refreshTransactionHistory(txInfo.fromAddress)
      await store.dispatch(PanelActions.setSelectedTransaction(txInfo))
      await store.dispatch(PanelActions.navigateTo('transactionDetails'))
      apiProxy.panelHandler.setCloseOnDeactivate(true)
      return
    }

    if (code !== undefined) {
      if (code === 'unauthorized') {
        await store.dispatch(PanelActions.setHardwareWalletInteractionError(code))
        return
      }

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
  } else if (hardwareAccount.vendor === BraveWallet.TREZOR_HARDWARE_VENDOR) {
    const { success, error, deviceError } = await signTrezorTransaction(apiProxy, hardwareAccount.path, txInfo)
    if (success) {
      refreshTransactionHistory(txInfo.fromAddress)
      await store.dispatch(PanelActions.setSelectedTransaction(txInfo))
      await store.dispatch(PanelActions.navigateTo('transactionDetails'))
      apiProxy.panelHandler.setCloseOnDeactivate(true)
      // By default the focus is moved to the browser window automatically when
      // Trezor popup closed which triggers an OnDeactivate event that would
      // close the wallet panel because of the above API call. However, there
      // could be times that the above call happens after OnDeactivate event, so
      // the wallet panel would stay open after Trezor popup closed.
      // As a workaround, we manually set the focus back to wallet panel here so
      // it would trigger another OnDeactivate event when user clicks outside
      // of the wallet panel.
      apiProxy.panelHandler.focus()
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

handler.on(PanelActions.connectToSite.type, async (store: Store, payload: ConnectWithSitePayloadType) => {
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.connectToSite([payload.addressToConnect], payload.duration)
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.visibilityChanged.type, async (store: Store, isVisible) => {
  if (!isVisible) {
    return
  }
  await refreshWalletInfo(store)
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.showConnectToSite.type, async (store: Store, payload: ShowConnectToSitePayload) => {
  store.dispatch(PanelActions.navigateTo('connectWithSite'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.showApproveTransaction.type, async (store: Store, payload: ShowConnectToSitePayload) => {
  store.dispatch(PanelActions.navigateTo('approveTransaction'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.showUnlock.type, async (store: Store) => {
  store.dispatch(PanelActions.navigateTo('showUnlock'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.addEthereumChain.type, async (store: Store, request: BraveWallet.AddChainRequest) => {
  store.dispatch(PanelActions.navigateTo('addEthereumChain'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.addEthereumChainRequestCompleted.type, async (store: any, payload: EthereumChainRequestPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const jsonRpcService = apiProxy.jsonRpcService
  jsonRpcService.addEthereumChainRequestCompleted(payload.chainId, payload.approved)
  const request = await getPendingAddChainRequest()
  if (request) {
    store.dispatch(PanelActions.addEthereumChain(request))
    return
  }
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.switchEthereumChain.type, async (store: Store, request: SerializableSwitchChainRequest) => {
  // We need to get current network list first because switch chain doesn't
  // require permission connect first.
  await refreshWalletInfo(store)
  store.dispatch(PanelActions.navigateTo('switchEthereumChain'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.getEncryptionPublicKey.type, async (store: Store) => {
  store.dispatch(PanelActions.navigateTo('provideEncryptionKey'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.decrypt.type, async (store: Store) => {
  store.dispatch(PanelActions.navigateTo('allowReadingEncryptedMessage'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.switchEthereumChainProcessed.type, async (store: Store, payload: SwitchEthereumChainProcessedPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const jsonRpcService = apiProxy.jsonRpcService
  jsonRpcService.notifySwitchChainRequestProcessed(payload.approved, deserializeOrigin(payload.origin))
  const switchChainRequest = await getPendingSwitchChainRequest()
  if (switchChainRequest) {
    store.dispatch(PanelActions.switchEthereumChain(
      makeSerializableSwitchChainRequest(switchChainRequest)
    ))
    return
  }
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.getEncryptionPublicKeyProcessed.type, async (store: Store, payload: GetEncryptionPublicKeyProcessedPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const braveWalletService = apiProxy.braveWalletService
  braveWalletService.notifyGetPublicKeyRequestProcessed(payload.approved, deserializeOrigin(payload.origin))
  const getEncryptionPublicKeyRequest = await getPendingGetEncryptionPublicKeyRequest()
  if (getEncryptionPublicKeyRequest) {
    store.dispatch(PanelActions.getEncryptionPublicKey(
      makeSerializableGetEncryptionPublicKeyRequest(getEncryptionPublicKeyRequest)
    ))
    return
  }
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.decryptProcessed.type, async (store: Store, payload: DecryptProcessedPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const braveWalletService = apiProxy.braveWalletService
  braveWalletService.notifyDecryptRequestProcessed(payload.approved, deserializeOrigin(payload.origin))
  const decryptRequest = await getPendingDecryptRequest()
  if (decryptRequest) {
    store.dispatch(PanelActions.decrypt(
      makeSerializableDecryptRequest(decryptRequest)
    ))
    return
  }
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.signMessage.type, async (store: Store) => {
  store.dispatch(PanelActions.navigateTo('signData'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.signMessageProcessed.type, async (store: Store, payload: SignMessageProcessedPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const braveWalletService = apiProxy.braveWalletService
  braveWalletService.notifySignMessageRequestProcessed(payload.approved, payload.id, null, null)
  const signMessageRequests = await getPendingSignMessageRequests()
  if (signMessageRequests) {
    store.dispatch(PanelActions.signMessage(
      signMessageRequests.map(makeSerializableSignMessageRequest)
    ))
    return
  }
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.signMessageHardware.type, async (store, messageData: SerializableSignMessageRequest) => {
  const apiProxy = getWalletPanelApiProxy()
  const hardwareAccount = await findHardwareAccountInfo(messageData.address)
  if (!hardwareAccount || !hardwareAccount.hardware) {
    const braveWalletService = apiProxy.braveWalletService
    braveWalletService.notifySignMessageRequestProcessed(false, messageData.id,
      null, getLocale('braveWalletHardwareAccountNotFound'))
    const signMessageRequests = await getPendingSignMessageRequests()
    if (signMessageRequests) {
      store.dispatch(PanelActions.signMessage(
        signMessageRequests.map(makeSerializableSignMessageRequest)
      ))
      return
    }
    apiProxy.panelHandler.closeUI()
    return
  }
  await navigateToConnectHardwareWallet(store)
  const info = hardwareAccount.hardware
  const signed = await signMessageWithHardwareKeyring(
    // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
    info.vendor as HardwareVendor,
    info.path,
    messageData
  )
  if (!signed.success && signed.code) {
    if (signed.code === 'unauthorized') {
      await store.dispatch(PanelActions.setHardwareWalletInteractionError(signed.code))
      return
    }
    const deviceError = (info.vendor === BraveWallet.TREZOR_HARDWARE_VENDOR)
      ? dialogErrorFromTrezorErrorCode(signed.code) : dialogErrorFromLedgerErrorCode(signed.code)
    if (deviceError !== 'transactionRejected') {
      await store.dispatch(PanelActions.setHardwareWalletInteractionError(deviceError))
      return
    }
  }
  let signature: BraveWallet.ByteArrayStringUnion | undefined
  if (signed.success) {
    // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
    signature = hardwareAccount.coin === BraveWallet.CoinType.SOL
      ? { bytes: signed.payload as Buffer } : { str: signed.payload as string }
  }
  const payload: SignMessageProcessedPayload =
    signed.success
      ? { approved: signed.success, id: messageData.id, signature: signature }
      : { approved: signed.success, id: messageData.id, error: signed.error as (string | undefined) }
  store.dispatch(PanelActions.signMessageHardwareProcessed(payload))
  await store.dispatch(PanelActions.navigateToMain())
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.signMessageHardwareProcessed.type, async (store, payload: SignMessageProcessedPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const braveWalletService = apiProxy.braveWalletService
  braveWalletService.notifySignMessageRequestProcessed(payload.approved, payload.id, payload.signature || null, payload.error || null)
  const signMessageRequests = await getPendingSignMessageRequests()
  if (signMessageRequests) {
    store.dispatch(PanelActions.signMessage(
      signMessageRequests.map(makeSerializableSignMessageRequest)
    ))
    return
  }
  apiProxy.panelHandler.closeUI()
})

// Sign Transaction
handler.on(PanelActions.signTransaction.type, async (store: Store, payload: BraveWallet.SignTransactionRequest[]) => {
  store.dispatch(PanelActions.navigateTo('signTransaction'))
  const { panelHandler } = getWalletPanelApiProxy()
  panelHandler.showUI()
})

handler.on(PanelActions.signTransactionHardware.type, async (store, messageData: BraveWallet.SignTransactionRequest) => {
  const apiProxy = getWalletPanelApiProxy()
  const hardwareAccount = await findHardwareAccountInfo(messageData.fromAddress)
  if (!hardwareAccount || !hardwareAccount.hardware) {
    const braveWalletService = apiProxy.braveWalletService
    braveWalletService.notifySignTransactionRequestProcessed(false, messageData.id,
      null, getLocale('braveWalletHardwareAccountNotFound'))
    const requests = await getPendingSignTransactionRequests()
    if (requests) {
      store.dispatch(PanelActions.signTransaction(requests))
      return
    }
    apiProxy.panelHandler.closeUI()
    return
  }

  await navigateToConnectHardwareWallet(store)
  const info = hardwareAccount.hardware
  // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
  const signed = await signRawTransactionWithHardwareKeyring(info.vendor as HardwareVendor, info.path, messageData.rawMessage, messageData.coin, () => {
    store.dispatch(PanelActions.signTransaction([messageData]))
  })
  if (signed?.code === 'unauthorized') {
    await store.dispatch(PanelActions.setHardwareWalletInteractionError(signed.code))
    return
  }
  let signature: BraveWallet.ByteArrayStringUnion | undefined
  if (signed.success) {
    // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
    signature = hardwareAccount.coin === BraveWallet.CoinType.SOL
      ? { bytes: signed.payload as Buffer } : { str: signed.payload as string }
  }
  const payload: SignMessageProcessedPayload =
    signed.success
      ? { approved: signed.success, id: messageData.id, signature: signature }
      : { approved: signed.success, id: messageData.id, error: signed.error as (string | undefined) }
  store.dispatch(PanelActions.signTransactionProcessed(payload))
  await store.dispatch(PanelActions.navigateToMain())
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.signTransactionProcessed.type, async (store: Store, payload: SignMessageProcessedPayload) => {
  const { braveWalletService, panelHandler } = getWalletPanelApiProxy()
  braveWalletService.notifySignTransactionRequestProcessed(payload.approved, payload.id, payload.signature || null, payload.error || null)
  const requests = await getPendingSignTransactionRequests()
  if (requests) {
    store.dispatch(PanelActions.signTransaction(requests))
    return
  }
  panelHandler.closeUI()
})

// Sign All Transactions
handler.on(PanelActions.signAllTransactions.type, async (store: Store, payload: BraveWallet.SignAllTransactionsRequest[]) => {
  store.dispatch(PanelActions.navigateTo('signAllTransactions'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.signAllTransactionsHardware.type, async (store, messageData: BraveWallet.SignAllTransactionsRequest) => {
  const apiProxy = getWalletPanelApiProxy()
  const hardwareAccount = await findHardwareAccountInfo(messageData.fromAddress)
  if (!hardwareAccount || !hardwareAccount.hardware) {
    const braveWalletService = apiProxy.braveWalletService
    braveWalletService.notifySignAllTransactionsRequestProcessed(false, messageData.id,
      null, getLocale('braveWalletHardwareAccountNotFound'))
    const requests = await getPendingSignAllTransactionsRequests()
    if (requests) {
      store.dispatch(PanelActions.signAllTransactions(requests))
      return
    }
    apiProxy.panelHandler.closeUI()
    return
  }

  await navigateToConnectHardwareWallet(store)
  const info = hardwareAccount.hardware
  // Send serialized requests to hardware keyring to sign.
  let payload: SignAllTransactionsProcessedPayload = { approved: true, id: messageData.id, signatures: [] }
  for (const rawMessage of messageData.rawMessages) {
    // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
    const signed = await signRawTransactionWithHardwareKeyring(info.vendor as HardwareVendor, info.path, rawMessage, messageData.coin, () => {
      store.dispatch(PanelActions.signAllTransactions([messageData]))
    })
    if (!signed.success) {
      if (signed.code && signed.code === 'unauthorized') {
        await store.dispatch(PanelActions.setHardwareWalletInteractionError(signed.code))
        return
      }
      payload.approved = false
      payload.signatures = undefined
      payload.error = signed.error as string
      break
    }
    // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
    const signature: BraveWallet.ByteArrayStringUnion = hardwareAccount.coin === BraveWallet.CoinType.SOL
      ? { bytes: signed.payload as Buffer } : { str: signed.payload as string }
    payload.signatures?.push(signature)
  }

  store.dispatch(PanelActions.signAllTransactionsProcessed(payload))
  await store.dispatch(PanelActions.navigateToMain())
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.signAllTransactionsProcessed.type, async (store: Store, payload: SignAllTransactionsProcessedPayload) => {
  const { braveWalletService, panelHandler } = getWalletPanelApiProxy()
  braveWalletService.notifySignAllTransactionsRequestProcessed(payload.approved, payload.id, payload.signatures || null, payload.error || null)
  const requests = await getPendingSignAllTransactionsRequests()
  if (requests) {
    store.dispatch(PanelActions.signAllTransactions(requests))
    return
  }
  panelHandler.closeUI()
})

handler.on(PanelActions.addSuggestToken.type, async (store: Store, payload: BraveWallet.AddSuggestTokenRequest[]) => {
  store.dispatch(PanelActions.navigateTo('addSuggestedToken'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(PanelActions.addSuggestTokenProcessed.type, async (store: Store, payload: AddSuggestTokenProcessedPayload) => {
  const apiProxy = getWalletPanelApiProxy()
  const braveWalletService = apiProxy.braveWalletService
  braveWalletService.notifyAddSuggestTokenRequestsProcessed(payload.approved, [payload.contractAddress])
  const addSuggestTokenRequest = await getPendingAddSuggestTokenRequest()
  if (addSuggestTokenRequest) {
    store.dispatch(PanelActions.addSuggestToken(addSuggestTokenRequest))
    return
  }
  apiProxy.panelHandler.closeUI()
})

handler.on(PanelActions.showApproveTransaction.type, async (store) => {
  store.dispatch(PanelActions.navigateTo('approveTransaction'))
})

handler.on(PanelActions.setupWallet.type, async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.expandWallet.type, async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet/crypto' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.openWalletApps.type, async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet/crypto/apps' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.expandRestoreWallet.type, async (store) => {
  chrome.tabs.create({ url: `chrome://wallet${WalletRoutes.Restore}` }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.expandWalletAccounts.type, async (store) => {
  chrome.tabs.create({ url: `chrome://wallet${WalletRoutes.AddAccountModal}` }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.expandWalletAddAsset.type, async (store) => {
  chrome.tabs.create({ url: `chrome://wallet${WalletRoutes.AddAssetModal}` }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(PanelActions.openWalletSettings.type, async (store) => {
  chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

// Cross-slice action handlers
handler.on(WalletActions.initialize.type, async (store) => {
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
    const originScheme = url.searchParams.get('origin-scheme') || ''
    const originHost = url.searchParams.get('origin-host') || ''
    const originPort = Number(url.searchParams.get('origin-port') || 0)
    const originSpec = url.searchParams.get('origin-spec') || ''
    const eTldPlusOne = url.searchParams.get('etld-plus-one') || ''
    const originInfo: BraveWallet.OriginInfo = {
      origin: {
        scheme: originScheme,
        host: originHost,
        port: originPort,
        nonceIfOpaque: undefined
      },
      originSpec: originSpec,
      eTldPlusOne: eTldPlusOne
    }

    store.dispatch(PanelActions.showConnectToSite({ accounts, originInfo }))
    return
  } else {
    const unlockRequest = await hasPendingUnlockRequest()
    if (unlockRequest) {
      store.dispatch(PanelActions.showUnlock())
    }
    const addChainRequest = await getPendingAddChainRequest()
    if (addChainRequest) {
      store.dispatch(PanelActions.addEthereumChain(addChainRequest))
      return
    }

    const signTransactionRequests = await getPendingSignTransactionRequests()
    if (signTransactionRequests) {
      store.dispatch(PanelActions.signTransaction(signTransactionRequests))
      return
    }

    const signAllTransactionsRequests = await getPendingSignAllTransactionsRequests()
    if (signAllTransactionsRequests) {
      store.dispatch(PanelActions.signAllTransactions(signAllTransactionsRequests))
      return
    }

    const signMessageRequests = await getPendingSignMessageRequests()
    if (signMessageRequests) {
      store.dispatch(PanelActions.signMessage(
        signMessageRequests.map(makeSerializableSignMessageRequest)
      ))
      return
    }
    const switchChainRequest = await getPendingSwitchChainRequest()
    if (switchChainRequest) {
      store.dispatch(PanelActions.switchEthereumChain(
        makeSerializableSwitchChainRequest(switchChainRequest)
      ))
      return
    }
    const addSuggestTokenRequest = await getPendingAddSuggestTokenRequest()
    if (addSuggestTokenRequest) {
      store.dispatch(PanelActions.addSuggestToken(addSuggestTokenRequest))
      return
    }
    const getEncryptionPublicKeyRequest = await getPendingGetEncryptionPublicKeyRequest()
    if (getEncryptionPublicKeyRequest) {
      store.dispatch(PanelActions.getEncryptionPublicKey(
        makeSerializableGetEncryptionPublicKeyRequest(getEncryptionPublicKeyRequest)
      ))
      return
    }
    const decryptRequest = await getPendingDecryptRequest()
    if (decryptRequest) {
      store.dispatch(PanelActions.decrypt(
        makeSerializableDecryptRequest(decryptRequest)
      ))
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

handler.on(WalletActions.transactionStatusChanged.type, async (store: Store, payload: TransactionStatusChanged) => {
  const state = getPanelState(store)
  const walletState = getWalletState(store)
  if (
    [BraveWallet.TransactionStatus.Submitted,
      BraveWallet.TransactionStatus.Signed,
      BraveWallet.TransactionStatus.Rejected,
      BraveWallet.TransactionStatus.Approved]
      .includes(payload.txInfo.txStatus)
  ) {
    if ((
      state.selectedPanel === 'approveTransaction' ||
      payload.txInfo.txStatus === BraveWallet.TransactionStatus.Rejected
    ) && walletState.pendingTransactions.length === 0
    ) {
      const apiProxy = getWalletPanelApiProxy()
      apiProxy.panelHandler.closeUI()
    }
  }
})

handler.on(WalletActions.unlocked.type, async (store: Store) => {
  const state = getPanelState(store)
  if (state.selectedPanel === 'showUnlock') {
    const apiProxy = getWalletPanelApiProxy()
    apiProxy.panelHandler.closeUI()
  }
})

export default handler.middleware
