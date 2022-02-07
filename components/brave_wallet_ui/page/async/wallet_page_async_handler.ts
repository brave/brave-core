// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import getWalletPageApiProxy from '../wallet_page_api_proxy'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletPageActions from '../actions/wallet_page_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import {
  BraveWallet,
  UpdateAccountNamePayloadType,
  WalletState
} from '../../constants/types'
import {
  CreateWalletPayloadType,
  RestoreWalletPayloadType,
  UpdateSelectedAssetType,
  AddAccountPayloadType,
  ImportAccountPayloadType,
  RemoveImportedAccountPayloadType,
  RemoveHardwareAccountPayloadType,
  ViewPrivateKeyPayloadType,
  ImportAccountFromJsonPayloadType,
  ImportFromExternalWalletPayloadType,
  ImportFilecoinAccountPayloadType
} from '../constants/action_types'
import {
  findHardwareAccountInfo,
  getKeyringIdFromAddress
} from '../../common/async/lib'
import { NewUnapprovedTxAdded } from '../../common/constants/action_types'
import { fetchSwapQuoteFactory } from '../../common/async/handlers'
import { Store } from '../../common/async/types'
import { GetTokenParam } from '../../utils/api-utils'
import { extractPublicKeyForBLS, encodeKeyToHex } from '../../common/hardware/ledgerjs/filecoin_ledger_keyring'

const handler = new AsyncActionHandler()

function getWalletState (store: Store): WalletState {
  return store.getState().wallet
}

async function refreshWalletInfo (store: Store) {
  const walletHandler = getWalletPageApiProxy().walletHandler
  const result = await walletHandler.getWalletInfo()
  store.dispatch(WalletActions.initialized({ ...result, selectedAccount: '', visibleTokens: [] }))
}

handler.on(WalletPageActions.createWallet.getType(), async (store: Store, payload: CreateWalletPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.createWallet(payload.password)
  store.dispatch(WalletPageActions.walletCreated({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.restoreWallet.getType(), async (store: Store, payload: RestoreWalletPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.restoreWallet(payload.mnemonic, payload.password, payload.isLegacy)
  if (!result.isValidMnemonic) {
    store.dispatch(WalletPageActions.hasMnemonicError(!result.isValidMnemonic))
    return
  }
  keyringService.notifyWalletBackupComplete()
  await refreshWalletInfo(store)
  store.dispatch(WalletPageActions.setShowIsRestoring(false))
})

handler.on(WalletPageActions.addAccount.getType(), async (store: Store, payload: AddAccountPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.addAccount(payload.accountName, payload.coin)
  return result.success
})

handler.on(WalletPageActions.showRecoveryPhrase.getType(), async (store: Store, payload: boolean) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.getMnemonicForDefaultKeyring()
  store.dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.walletBackupComplete.getType(), async (store) => {
  const keyringService = getWalletPageApiProxy().keyringService
  keyringService.notifyWalletBackupComplete()
})

handler.on(WalletPageActions.selectAsset.getType(), async (store: Store, payload: UpdateSelectedAssetType) => {
  store.dispatch(WalletPageActions.updateSelectedAsset(payload.asset))
  store.dispatch(WalletPageActions.setIsFetchingPriceHistory(true))
  const assetRatioService = getWalletPageApiProxy().assetRatioService
  const walletState = getWalletState(store)
  const defaultFiat = walletState.defaultCurrencies.fiat.toLowerCase()
  const defaultCrypto = walletState.defaultCurrencies.crypto.toLowerCase()
  const selectedNetwork = walletState.selectedNetwork
  if (payload.asset) {
    const selectedAsset = payload.asset
    const defaultPrices = await assetRatioService.getPrice([GetTokenParam(selectedNetwork, selectedAsset)], [defaultFiat, defaultCrypto], payload.timeFrame)
    const priceHistory = await assetRatioService.getPriceHistory(GetTokenParam(selectedNetwork, selectedAsset), defaultFiat, payload.timeFrame)
    store.dispatch(WalletPageActions.updatePriceInfo({ priceHistory: priceHistory, defaultFiatPrice: defaultPrices.values[0], defaultCryptoPrice: defaultPrices.values[1], timeFrame: payload.timeFrame }))
  } else {
    store.dispatch(WalletPageActions.updatePriceInfo({ priceHistory: undefined, defaultFiatPrice: undefined, defaultCryptoPrice: undefined, timeFrame: payload.timeFrame }))
  }
})

handler.on(WalletPageActions.importAccount.getType(), async (store: Store, payload: ImportAccountPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.importAccount(payload.accountName, payload.privateKey, payload.coin)
  if (result.success) {
    store.dispatch(WalletPageActions.setImportAccountError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportAccountError(true))
  }
})

handler.on(WalletPageActions.importFilecoinAccount.getType(), async (store: Store, payload: ImportFilecoinAccountPayloadType) => {
  const { keyringService } = getWalletPageApiProxy()
  const result = (payload.protocol === BraveWallet.FilecoinAddressProtocol.SECP256K1)
    ? await keyringService.importFilecoinSECP256K1Account(payload.accountName, encodeKeyToHex(payload.privateKey), payload.network)
    : await keyringService.importFilecoinBLSAccount(payload.accountName, payload.privateKey, extractPublicKeyForBLS(payload.privateKey), payload.network)

  if (result.success) {
    store.dispatch(WalletPageActions.setImportAccountError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportAccountError(true))
  }
})

handler.on(WalletPageActions.importAccountFromJson.getType(), async (store: Store, payload: ImportAccountFromJsonPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.importAccountFromJson(payload.accountName, payload.password, payload.json)
  if (result.success) {
    store.dispatch(WalletPageActions.setImportAccountError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportAccountError(true))
  }
})

handler.on(WalletPageActions.removeImportedAccount.getType(), async (store: Store, payload: RemoveImportedAccountPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.removeImportedAccount(payload.address)
  return result.success
})

handler.on(WalletPageActions.viewPrivateKey.getType(), async (store: Store, payload: ViewPrivateKeyPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = payload.isDefault
    ? await keyringService.getPrivateKeyForDefaultKeyringAccount(payload.address)
    : await keyringService.getPrivateKeyForImportedAccount(payload.address)
  store.dispatch(WalletPageActions.privateKeyAvailable({ privateKey: result.privateKey }))
})

handler.on(WalletPageActions.updateAccountName.getType(), async (store: Store, payload: UpdateAccountNamePayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const hardwareAccount = await findHardwareAccountInfo(payload.address)
  if (hardwareAccount && hardwareAccount.hardware) {
    const result = await keyringService.setDefaultKeyringHardwareAccountName(payload.address, payload.name)
    return result.success
  }
  const keyringId = await getKeyringIdFromAddress(payload.address)
  const result = payload.isDerived
    ? await keyringService.setKeyringDerivedAccountName(keyringId, payload.address, payload.name)
    : await keyringService.setKeyringImportedAccountName(keyringId, payload.address, payload.name)
  return result.success
})

handler.on(WalletPageActions.addHardwareAccounts.getType(), async (store: Store, accounts: BraveWallet.HardwareWalletAccount[]) => {
  const keyringService = getWalletPageApiProxy().keyringService
  keyringService.addHardwareAccounts(accounts)
  store.dispatch(WalletPageActions.setShowAddModal(false))
})

handler.on(WalletPageActions.removeHardwareAccount.getType(), async (store: Store, payload: RemoveHardwareAccountPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  keyringService.removeHardwareAccount(payload.address)
  store.dispatch(WalletPageActions.setShowAddModal(false))
})

handler.on(WalletPageActions.checkWalletsToImport.getType(), async (store) => {
  const braveWalletService = getWalletPageApiProxy().braveWalletService
  const cwResult =
    await braveWalletService.isExternalWalletInitialized(
      BraveWallet.ExternalWalletType.CryptoWallets)
  const mmResult =
    await braveWalletService.isExternalWalletInitialized(
      BraveWallet.ExternalWalletType.MetaMask)
  store.dispatch(WalletPageActions.setCryptoWalletsInitialized(cwResult.initialized))
  store.dispatch(WalletPageActions.setMetaMaskInitialized(mmResult.initialized))
})

handler.on(WalletPageActions.importFromCryptoWallets.getType(), async (store: Store, payload: ImportFromExternalWalletPayloadType) => {
  const braveWalletService = getWalletPageApiProxy().braveWalletService
  const keyringService = getWalletPageApiProxy().keyringService
  const result =
    await braveWalletService.importFromExternalWallet(
      BraveWallet.ExternalWalletType.CryptoWallets, payload.password, payload.newPassword)
  if (result.success) {
    keyringService.notifyWalletBackupComplete()
  }
  store.dispatch(WalletPageActions.setImportWalletError({
    hasError: !result.success,
    errorMessage: result.errorMessage ?? undefined
  }))
})

handler.on(WalletPageActions.importFromMetaMask.getType(), async (store: Store, payload: ImportFromExternalWalletPayloadType) => {
  const braveWalletService = getWalletPageApiProxy().braveWalletService
  const keyringService = getWalletPageApiProxy().keyringService
  const result =
    await braveWalletService.importFromExternalWallet(
      BraveWallet.ExternalWalletType.MetaMask, payload.password, payload.newPassword)
  if (result.success) {
    keyringService.notifyWalletBackupComplete()
  }
  store.dispatch(WalletPageActions.setImportWalletError({
    hasError: !result.success,
    errorMessage: result.errorMessage ?? undefined
  }))
})

handler.on(WalletActions.newUnapprovedTxAdded.getType(), async (store: Store, payload: NewUnapprovedTxAdded) => {
  const pageHandler = getWalletPageApiProxy().pageHandler
  pageHandler.showApprovePanelUI()
})

handler.on(
  WalletPageActions.fetchPageSwapQuote.getType(),
  fetchSwapQuoteFactory(WalletPageActions.setPageSwapQuote, WalletPageActions.setPageSwapError)
)

handler.on(WalletPageActions.openWalletSettings.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

export default handler.middleware
