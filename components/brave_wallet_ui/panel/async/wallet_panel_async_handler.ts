// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assert } from 'chrome://resources/js/assert.js'

import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as PanelActions from '../actions/wallet_panel_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import {
  BraveWallet,
  WalletPanelState,
  PanelState,
  PanelTypes
} from '../../constants/types'
import {
  ShowConnectToSitePayload,
  EthereumChainRequestPayload,
  SignMessageProcessedPayload,
  SignAllTransactionsProcessedPayload,
  SwitchEthereumChainProcessedPayload,
  GetEncryptionPublicKeyProcessedPayload,
  DecryptProcessedPayload,
  SignTransactionHardwarePayload,
  SignAllTransactionsHardwarePayload,
  SignMessageHardwarePayload
} from '../constants/action_types'
import {
  signMessageWithHardwareKeyring,
  signRawTransactionWithHardwareKeyring,
  cancelHardwareOperation,
  dialogErrorFromLedgerErrorCode,
  dialogErrorFromTrezorErrorCode
} from '../../common/async/hardware'

import { Store } from '../../common/async/types'
import { getLocale } from '../../../common/locale'
import getWalletPanelApiProxy from '../wallet_panel_api_proxy'
import { isHardwareAccount } from '../../utils/account-utils'
import { HardwareVendor } from 'components/brave_wallet_ui/common/api/hardware_keyrings'
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
  store.dispatch(
    WalletActions.refreshAll({
      skipBalancesRefresh: true
    })
  )
}

async function hasPendingUnlockRequest() {
  const keyringService = getWalletPanelApiProxy().keyringService
  return (await keyringService.hasPendingUnlockRequest()).pending
}

async function getPendingAddChainRequest() {
  const jsonRpcService = getWalletPanelApiProxy().jsonRpcService
  const requests = (await jsonRpcService.getPendingAddChainRequests()).requests
  if (requests && requests.length) {
    return requests[0]
  }
  return null
}

async function getPendingSwitchChainRequest() {
  const jsonRpcService = getWalletPanelApiProxy().jsonRpcService
  const requests = (await jsonRpcService.getPendingSwitchChainRequests())
    .requests
  if (requests && requests.length) {
    return requests[0]
  }
  return null
}

async function getPendingGetEncryptionPublicKeyRequest() {
  const braveWalletService = getWalletPanelApiProxy().braveWalletService
  const requests = (
    await braveWalletService.getPendingGetEncryptionPublicKeyRequests()
  ).requests
  if (requests && requests.length) {
    return requests[0]
  }
  return null
}

async function getPendingDecryptRequest() {
  const braveWalletService = getWalletPanelApiProxy().braveWalletService
  const requests = (await braveWalletService.getPendingDecryptRequests())
    .requests
  if (requests && requests.length) {
    return requests[0]
  }
  return null
}

async function getPendingSignMessageRequests() {
  const braveWalletService = getWalletPanelApiProxy().braveWalletService
  const requests = (await braveWalletService.getPendingSignMessageRequests())
    .requests
  if (requests && requests.length) {
    return requests
  }
  return null
}

async function getPendingSignMessageErrors() {
  const braveWalletService = getWalletPanelApiProxy().braveWalletService
  const errors = (await braveWalletService.getPendingSignMessageErrors()).errors
  if (errors && errors.length) {
    return errors
  }
  return null
}

async function getPendingSignTransactionRequests() {
  const { braveWalletService } = getWalletPanelApiProxy()
  const { requests } =
    await braveWalletService.getPendingSignTransactionRequests()

  if (requests && requests.length) {
    return requests
  }
  return null
}

async function getPendingSignAllTransactionsRequests() {
  const { braveWalletService } = getWalletPanelApiProxy()
  const { requests } =
    await braveWalletService.getPendingSignAllTransactionsRequests()

  if (requests && requests.length) {
    return requests
  }
  return null
}

async function navigateToConnectHardwareWallet(store: Store) {
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.setCloseOnDeactivate(false)

  const { selectedPanel } = getPanelState(store)
  if (selectedPanel === 'connectHardwareWallet') {
    return
  }

  await store.dispatch(PanelActions.navigateTo('connectHardwareWallet'))
  await store.dispatch(
    PanelActions.setHardwareWalletInteractionError(undefined)
  )
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

handler.on(PanelActions.showUnlock.type, async (store: Store) => {
  store.dispatch(PanelActions.navigateTo('showUnlock'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(
  PanelActions.addEthereumChain.type,
  async (store: Store, request: BraveWallet.AddChainRequest) => {
    store.dispatch(PanelActions.navigateTo('addEthereumChain'))
    const apiProxy = getWalletPanelApiProxy()
    apiProxy.panelHandler.showUI()
  }
)

handler.on(
  PanelActions.addEthereumChainRequestCompleted.type,
  async (store: any, payload: EthereumChainRequestPayload) => {
    const apiProxy = getWalletPanelApiProxy()
    const jsonRpcService = apiProxy.jsonRpcService
    jsonRpcService.addEthereumChainRequestCompleted(
      payload.chainId,
      payload.approved
    )
    const request = await getPendingAddChainRequest()
    if (request) {
      store.dispatch(PanelActions.addEthereumChain(request))
      return
    }
    apiProxy.panelHandler.closeUI()
  }
)

handler.on(
  PanelActions.switchEthereumChain.type,
  async (store: Store, request: BraveWallet.SwitchChainRequest) => {
    // We need to get current network list first because switch chain doesn't
    // require permission connect first.
    await refreshWalletInfo(store)
    store.dispatch(PanelActions.navigateTo('switchEthereumChain'))
    const apiProxy = getWalletPanelApiProxy()
    apiProxy.panelHandler.showUI()
  }
)

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

handler.on(
  PanelActions.switchEthereumChainProcessed.type,
  async (store: Store, payload: SwitchEthereumChainProcessedPayload) => {
    const apiProxy = getWalletPanelApiProxy()
    const jsonRpcService = apiProxy.jsonRpcService
    jsonRpcService.notifySwitchChainRequestProcessed(
      payload.requestId,
      payload.approved
    )
    const switchChainRequest = await getPendingSwitchChainRequest()
    if (switchChainRequest) {
      store.dispatch(PanelActions.switchEthereumChain(switchChainRequest))
      return
    }
    apiProxy.panelHandler.closeUI()
  }
)

handler.on(
  PanelActions.getEncryptionPublicKeyProcessed.type,
  async (store: Store, payload: GetEncryptionPublicKeyProcessedPayload) => {
    const apiProxy = getWalletPanelApiProxy()
    const braveWalletService = apiProxy.braveWalletService
    braveWalletService.notifyGetPublicKeyRequestProcessed(
      payload.requestId,
      payload.approved
    )
    const getEncryptionPublicKeyRequest =
      await getPendingGetEncryptionPublicKeyRequest()
    if (getEncryptionPublicKeyRequest) {
      store.dispatch(
        PanelActions.getEncryptionPublicKey(getEncryptionPublicKeyRequest)
      )
      return
    }
    apiProxy.panelHandler.closeUI()
  }
)

handler.on(
  PanelActions.decryptProcessed.type,
  async (store: Store, payload: DecryptProcessedPayload) => {
    const apiProxy = getWalletPanelApiProxy()
    const braveWalletService = apiProxy.braveWalletService
    braveWalletService.notifyDecryptRequestProcessed(
      payload.requestId,
      payload.approved
    )
    const decryptRequest = await getPendingDecryptRequest()
    if (decryptRequest) {
      store.dispatch(PanelActions.decrypt(decryptRequest))
      return
    }
    apiProxy.panelHandler.closeUI()
  }
)

handler.on(PanelActions.signMessage.type, async (store: Store) => {
  store.dispatch(PanelActions.navigateTo('signData'))
  const apiProxy = getWalletPanelApiProxy()
  apiProxy.panelHandler.showUI()
})

handler.on(
  PanelActions.signMessageProcessed.type,
  async (store: Store, payload: SignMessageProcessedPayload) => {
    const apiProxy = getWalletPanelApiProxy()
    const braveWalletService = apiProxy.braveWalletService
    braveWalletService.notifySignMessageRequestProcessed(
      payload.approved,
      payload.id,
      null,
      null
    )
    const signMessageRequests = await getPendingSignMessageRequests()
    if (signMessageRequests) {
      store.dispatch(PanelActions.signMessage(signMessageRequests))
      return
    }
    apiProxy.panelHandler.closeUI()
  }
)

function toByteArrayStringUnion<
  D extends keyof BraveWallet.ByteArrayStringUnion
>(unionItem: Pick<BraveWallet.ByteArrayStringUnion, D>) {
  return Object.assign({}, unionItem) as BraveWallet.ByteArrayStringUnion
}

handler.on(
  PanelActions.signMessageHardware.type,
  async (store, messageData: SignMessageHardwarePayload) => {
    const apiProxy = getWalletPanelApiProxy()
    if (!isHardwareAccount(messageData.account.accountId)) {
      const braveWalletService = apiProxy.braveWalletService
      braveWalletService.notifySignMessageRequestProcessed(
        false,
        messageData.request.id,
        null,
        getLocale('braveWalletHardwareAccountNotFound')
      )
      const signMessageRequests = await getPendingSignMessageRequests()
      if (signMessageRequests) {
        store.dispatch(PanelActions.signMessage(signMessageRequests))
        return
      }
      apiProxy.panelHandler.closeUI()
      return
    }
    await navigateToConnectHardwareWallet(store)
    const coin = messageData.account.accountId.coin
    const info = messageData.account.hardware
    assert(info)

    const signed = await signMessageWithHardwareKeyring(
      // eslint-disable @typescript-eslint/no-unnecessary-type-assertion
      info.vendor as HardwareVendor,
      // eslint-enable @typescript-eslint/no-unnecessary-type-assertion
      info.path,
      messageData.request
    )
    if (!signed.success && signed.code) {
      if (signed.code === 'unauthorized') {
        await store.dispatch(
          PanelActions.setHardwareWalletInteractionError(signed.code)
        )
        return
      }
      const deviceError =
        info.vendor === BraveWallet.TREZOR_HARDWARE_VENDOR
          ? dialogErrorFromTrezorErrorCode(signed.code)
          : dialogErrorFromLedgerErrorCode(signed.code)
      if (deviceError !== 'transactionRejected') {
        await store.dispatch(
          PanelActions.setHardwareWalletInteractionError(deviceError)
        )
        return
      }
    }
    let signature: BraveWallet.ByteArrayStringUnion | undefined
    if (signed.success) {
      signature =
        coin === BraveWallet.CoinType.SOL
          ? toByteArrayStringUnion({ bytes: [...(signed.payload as Buffer)] })
          : toByteArrayStringUnion({ str: signed.payload as string })
    }
    const payload: SignMessageProcessedPayload = signed.success
      ? {
          approved: signed.success,
          id: messageData.request.id,
          signature: signature
        }
      : {
          approved: signed.success,
          id: messageData.request.id,
          error: signed.error as string | undefined
        }
    store.dispatch(PanelActions.signMessageHardwareProcessed(payload))
    await store.dispatch(PanelActions.navigateToMain())
    apiProxy.panelHandler.closeUI()
  }
)

handler.on(
  PanelActions.signMessageHardwareProcessed.type,
  async (store, payload: SignMessageProcessedPayload) => {
    const apiProxy = getWalletPanelApiProxy()
    const braveWalletService = apiProxy.braveWalletService
    braveWalletService.notifySignMessageRequestProcessed(
      payload.approved,
      payload.id,
      payload.signature || null,
      payload.error || null
    )
    const signMessageRequests = await getPendingSignMessageRequests()
    if (signMessageRequests) {
      store.dispatch(PanelActions.signMessage(signMessageRequests))
      return
    }
    apiProxy.panelHandler.closeUI()
  }
)

// Sign Transaction
handler.on(
  PanelActions.signTransaction.type,
  async (store: Store, payload: BraveWallet.SignTransactionRequest[]) => {
    store.dispatch(PanelActions.navigateTo('signTransaction'))
    const { panelHandler } = getWalletPanelApiProxy()
    panelHandler.showUI()
  }
)

handler.on(
  PanelActions.signTransactionHardware.type,
  async (store, messageData: SignTransactionHardwarePayload) => {
    const apiProxy = getWalletPanelApiProxy()
    if (!isHardwareAccount(messageData.account.accountId)) {
      const braveWalletService = apiProxy.braveWalletService
      braveWalletService.notifySignTransactionRequestProcessed(
        false,
        messageData.request.id,
        null,
        getLocale('braveWalletHardwareAccountNotFound')
      )
      const requests = await getPendingSignTransactionRequests()
      if (requests) {
        store.dispatch(PanelActions.signTransaction(requests))
        return
      }
      apiProxy.panelHandler.closeUI()
      return
    }

    await navigateToConnectHardwareWallet(store)
    const coin = messageData.account.accountId.coin
    const info = messageData.account.hardware
    assert(info)

    const signed = await signRawTransactionWithHardwareKeyring(
      info.vendor as HardwareVendor,
      info.path,
      messageData.request.rawMessage,
      coin,
      () => {
        store.dispatch(PanelActions.signTransaction([messageData.request]))
      }
    )
    if (signed?.code === 'unauthorized') {
      await store.dispatch(
        PanelActions.setHardwareWalletInteractionError(signed.code)
      )
      return
    }
    let signature: BraveWallet.ByteArrayStringUnion | undefined
    if (signed.success) {
      signature =
        coin === BraveWallet.CoinType.SOL
          ? toByteArrayStringUnion({ bytes: [...(signed.payload as Buffer)] })
          : toByteArrayStringUnion({ str: signed.payload as string })
    }

    const payload: SignMessageProcessedPayload = signed.success
      ? {
          approved: signed.success,
          id: messageData.request.id,
          signature: signature
        }
      : {
          approved: signed.success,
          id: messageData.request.id,
          error: signed.error as string | undefined
        }
    store.dispatch(PanelActions.signTransactionProcessed(payload))
    await store.dispatch(PanelActions.navigateToMain())
    apiProxy.panelHandler.closeUI()
  }
)

handler.on(
  PanelActions.signTransactionProcessed.type,
  async (store: Store, payload: SignMessageProcessedPayload) => {
    const { braveWalletService, panelHandler } = getWalletPanelApiProxy()
    braveWalletService.notifySignTransactionRequestProcessed(
      payload.approved,
      payload.id,
      payload.signature || null,
      payload.error || null
    )
    const requests = await getPendingSignTransactionRequests()
    if (requests) {
      store.dispatch(PanelActions.signTransaction(requests))
      return
    }
    panelHandler.closeUI()
  }
)

// Sign All Transactions
handler.on(
  PanelActions.signAllTransactions.type,
  async (store: Store, payload: BraveWallet.SignAllTransactionsRequest[]) => {
    store.dispatch(PanelActions.navigateTo('signAllTransactions'))
    const apiProxy = getWalletPanelApiProxy()
    apiProxy.panelHandler.showUI()
  }
)

handler.on(
  PanelActions.signAllTransactionsHardware.type,
  async (store, messageData: SignAllTransactionsHardwarePayload) => {
    const apiProxy = getWalletPanelApiProxy()
    if (!isHardwareAccount(messageData.account.accountId)) {
      const braveWalletService = apiProxy.braveWalletService
      braveWalletService.notifySignAllTransactionsRequestProcessed(
        false,
        messageData.request.id,
        null,
        getLocale('braveWalletHardwareAccountNotFound')
      )
      const requests = await getPendingSignAllTransactionsRequests()
      if (requests) {
        store.dispatch(PanelActions.signAllTransactions(requests))
        return
      }
      apiProxy.panelHandler.closeUI()
      return
    }

    await navigateToConnectHardwareWallet(store)
    const coin = messageData.account.accountId.coin
    const info = messageData.account.hardware
    assert(info)

    // Send serialized requests to hardware keyring to sign.
    const payload: SignAllTransactionsProcessedPayload = {
      approved: true,
      id: messageData.request.id,
      signatures: []
    }
    for (const rawMessage of messageData.request.rawMessages) {
      const signed = await signRawTransactionWithHardwareKeyring(
        info.vendor as HardwareVendor,
        info.path,
        rawMessage,
        coin,
        () => {
          store.dispatch(
            PanelActions.signAllTransactions([messageData.request])
          )
        }
      )

      if (!signed.success) {
        if (signed.code && signed.code === 'unauthorized') {
          await store.dispatch(
            PanelActions.setHardwareWalletInteractionError(signed.code)
          )
          return
        }
        payload.approved = false
        payload.signatures = undefined
        payload.error = signed.error as string
        break
      }
      const signature: BraveWallet.ByteArrayStringUnion =
        coin === BraveWallet.CoinType.SOL
          ? toByteArrayStringUnion({ bytes: [...(signed.payload as Buffer)] })
          : toByteArrayStringUnion({ str: signed.payload as string })
      payload.signatures?.push(signature)
    }

    store.dispatch(PanelActions.signAllTransactionsProcessed(payload))
    await store.dispatch(PanelActions.navigateToMain())
    apiProxy.panelHandler.closeUI()
  }
)

handler.on(
  PanelActions.signAllTransactionsProcessed.type,
  async (store: Store, payload: SignAllTransactionsProcessedPayload) => {
    const { braveWalletService, panelHandler } = getWalletPanelApiProxy()
    braveWalletService.notifySignAllTransactionsRequestProcessed(
      payload.approved,
      payload.id,
      payload.signatures || null,
      payload.error || null
    )
    const requests = await getPendingSignAllTransactionsRequests()
    if (requests) {
      store.dispatch(PanelActions.signAllTransactions(requests))
      return
    }
    panelHandler.closeUI()
  }
)

handler.on(PanelActions.signMessageError.type, async (store: Store) => {
  const { panelHandler } = getWalletPanelApiProxy()
  panelHandler.showUI()
})

handler.on(
  PanelActions.signMessageErrorProcessed.type,
  async (store: Store, id: string) => {
    const { braveWalletService, panelHandler } = getWalletPanelApiProxy()
    braveWalletService.notifySignMessageErrorProcessed(id)
    const requests = await getPendingSignMessageErrors()
    if (requests) {
      store.dispatch(PanelActions.signMessageError(requests))
      return
    }
    panelHandler.closeUI()
  }
)

handler.on(PanelActions.setupWallet.type, async (store) => {
  chrome.tabs.create({ url: 'chrome://wallet' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

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

    const signAllTransactionsRequests =
      await getPendingSignAllTransactionsRequests()
    if (signAllTransactionsRequests) {
      store.dispatch(
        PanelActions.signAllTransactions(signAllTransactionsRequests)
      )
      return
    }

    const signMessageRequests = await getPendingSignMessageRequests()
    if (signMessageRequests) {
      store.dispatch(PanelActions.signMessage(signMessageRequests))
      return
    }

    const signMessageErrors = await getPendingSignMessageErrors()
    if (signMessageErrors) {
      store.dispatch(PanelActions.signMessageError(signMessageErrors))
      return
    }

    const switchChainRequest = await getPendingSwitchChainRequest()
    if (switchChainRequest) {
      store.dispatch(PanelActions.switchEthereumChain(switchChainRequest))
      return
    }

    const getEncryptionPublicKeyRequest =
      await getPendingGetEncryptionPublicKeyRequest()
    if (getEncryptionPublicKeyRequest) {
      store.dispatch(
        PanelActions.getEncryptionPublicKey(getEncryptionPublicKeyRequest)
      )
      return
    }
    const decryptRequest = await getPendingDecryptRequest()
    if (decryptRequest) {
      store.dispatch(PanelActions.decrypt(decryptRequest))
      return
    }
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

handler.on(WalletActions.unlocked.type, async (store: Store) => {
  const state = getPanelState(store)
  if (state.selectedPanel === 'showUnlock') {
    const apiProxy = getWalletPanelApiProxy()
    apiProxy.panelHandler.closeUI()
  }
})

export default handler.middleware
