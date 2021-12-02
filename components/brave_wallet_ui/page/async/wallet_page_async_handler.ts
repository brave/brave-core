// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import { FilecoinAddressProtocol } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
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
  findHardwareAccountInfo
} from '../../common/async/lib'
import { NewUnapprovedTxAdded } from '../../common/constants/action_types'
import { fetchSwapQuoteFactory } from '../../common/async/handlers'
import { Store } from '../../common/async/types'
import { GetTokenParam } from '../../utils/api-utils'
import { HardwareWalletAccount } from '../../common/hardware/types'
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
  const keyringController = getWalletPageApiProxy().keyringController
  const result = await keyringController.createWallet(payload.password)
  store.dispatch(WalletPageActions.walletCreated({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.restoreWallet.getType(), async (store: Store, payload: RestoreWalletPayloadType) => {
  const keyringController = getWalletPageApiProxy().keyringController
  const result = await keyringController.restoreWallet(payload.mnemonic, payload.password, payload.isLegacy)
  if (!result.isValidMnemonic) {
    store.dispatch(WalletPageActions.hasMnemonicError(!result.isValidMnemonic))
    return
  }
  keyringController.notifyWalletBackupComplete()
  await refreshWalletInfo(store)
  store.dispatch(WalletPageActions.setShowIsRestoring(false))
})

handler.on(WalletPageActions.addAccount.getType(), async (store: Store, payload: AddAccountPayloadType) => {
  const keyringController = getWalletPageApiProxy().keyringController
  const result = await keyringController.addAccount(payload.accountName)
  return result.success
})

handler.on(WalletPageActions.showRecoveryPhrase.getType(), async (store: Store, payload: boolean) => {
  const keyringController = getWalletPageApiProxy().keyringController
  const result = await keyringController.getMnemonicForDefaultKeyring()
  store.dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.walletBackupComplete.getType(), async (store) => {
  const keyringController = getWalletPageApiProxy().keyringController
  keyringController.notifyWalletBackupComplete()
})

handler.on(WalletPageActions.selectAsset.getType(), async (store: Store, payload: UpdateSelectedAssetType) => {
  store.dispatch(WalletPageActions.updateSelectedAsset(payload.asset))
  store.dispatch(WalletPageActions.setIsFetchingPriceHistory(true))
  const assetPriceController = getWalletPageApiProxy().assetRatioController
  const walletState = getWalletState(store)
  const defaultFiat = walletState.defaultCurrencies.fiat.toLowerCase()
  const defaultCrypto = walletState.defaultCurrencies.crypto.toLowerCase()
  const selectedNetwork = walletState.selectedNetwork
  if (payload.asset) {
    const selectedAsset = payload.asset
    const defaultPrices = await assetPriceController.getPrice([GetTokenParam(selectedNetwork, selectedAsset)], [defaultFiat, defaultCrypto], payload.timeFrame)
    const priceHistory = await assetPriceController.getPriceHistory(GetTokenParam(selectedNetwork, selectedAsset), defaultFiat, payload.timeFrame)
    store.dispatch(WalletPageActions.updatePriceInfo({ priceHistory: priceHistory, defaultFiatPrice: defaultPrices.values[0], defaultCryptoPrice: defaultPrices.values[1], timeFrame: payload.timeFrame }))
  } else {
    store.dispatch(WalletPageActions.updatePriceInfo({ priceHistory: undefined, defaultFiatPrice: undefined, defaultCryptoPrice: undefined, timeFrame: payload.timeFrame }))
  }
})

handler.on(WalletPageActions.importAccount.getType(), async (store: Store, payload: ImportAccountPayloadType) => {
  const keyringController = getWalletPageApiProxy().keyringController
  const result = await keyringController.importAccount(payload.accountName, payload.privateKey)
  if (result.success) {
    store.dispatch(WalletPageActions.setImportAccountError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportAccountError(true))
  }
})

handler.on(WalletPageActions.importFilecoinAccount.getType(), async (store: Store, payload: ImportFilecoinAccountPayloadType) => {
  const { keyringController } = getWalletPageApiProxy()
  const result = (payload.protocol === FilecoinAddressProtocol.SECP256K1)
    ? await keyringController.importFilecoinSECP256K1Account(payload.accountName, encodeKeyToHex(payload.privateKey), payload.network)
    : await keyringController.importFilecoinBLSAccount(payload.accountName, payload.privateKey, extractPublicKeyForBLS(payload.privateKey), payload.network)

  if (result.success) {
    store.dispatch(WalletPageActions.setImportAccountError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportAccountError(true))
  }
})

handler.on(WalletPageActions.importAccountFromJson.getType(), async (store: Store, payload: ImportAccountFromJsonPayloadType) => {
  const keyringController = getWalletPageApiProxy().keyringController
  const result = await keyringController.importAccountFromJson(payload.accountName, payload.password, payload.json)
  if (result.success) {
    store.dispatch(WalletPageActions.setImportAccountError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportAccountError(true))
  }
})

handler.on(WalletPageActions.removeImportedAccount.getType(), async (store: Store, payload: RemoveImportedAccountPayloadType) => {
  const keyringController = getWalletPageApiProxy().keyringController
  const result = await keyringController.removeImportedAccount(payload.address)
  return result.success
})

handler.on(WalletPageActions.viewPrivateKey.getType(), async (store: Store, payload: ViewPrivateKeyPayloadType) => {
  const keyringController = getWalletPageApiProxy().keyringController
  const result = payload.isDefault
    ? await keyringController.getPrivateKeyForDefaultKeyringAccount(payload.address)
    : await keyringController.getPrivateKeyForImportedAccount(payload.address)
  store.dispatch(WalletPageActions.privateKeyAvailable({ privateKey: result.privateKey }))
})

handler.on(WalletPageActions.updateAccountName.getType(), async (store: Store, payload: UpdateAccountNamePayloadType) => {
  const keyringController = getWalletPageApiProxy().keyringController
  const hardwareAccount = await findHardwareAccountInfo(payload.address)
  if (hardwareAccount && hardwareAccount.hardware) {
    const result = await keyringController.setDefaultKeyringHardwareAccountName(payload.address, payload.name)
    return result.success
  }

  const result = payload.isDerived
    ? await keyringController.setDefaultKeyringDerivedAccountName(payload.address, payload.name)
    : await keyringController.setDefaultKeyringImportedAccountName(payload.address, payload.name)
  return result.success
})

handler.on(WalletPageActions.addHardwareAccounts.getType(), async (store: Store, accounts: HardwareWalletAccount[]) => {
  const keyringController = getWalletPageApiProxy().keyringController
  keyringController.addHardwareAccounts(accounts)
  store.dispatch(WalletPageActions.setShowAddModal(false))
})

handler.on(WalletPageActions.removeHardwareAccount.getType(), async (store: Store, payload: RemoveHardwareAccountPayloadType) => {
  const keyringController = getWalletPageApiProxy().keyringController
  keyringController.removeHardwareAccount(payload.address)
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
  store.dispatch(WalletPageActions.setCryptoWalletsInstalled(cwResult.initialized))
  store.dispatch(WalletActions.setMetaMaskInstalled(mmResult.initialized))
})

handler.on(WalletPageActions.importFromCryptoWallets.getType(), async (store: Store, payload: ImportFromExternalWalletPayloadType) => {
  const braveWalletService = getWalletPageApiProxy().braveWalletService
  const keyringController = getWalletPageApiProxy().keyringController
  const result =
    await braveWalletService.importFromExternalWallet(
      BraveWallet.ExternalWalletType.CryptoWallets, payload.password, payload.newPassword)
  if (result.success) {
    keyringController.notifyWalletBackupComplete()
  }
  store.dispatch(WalletPageActions.setImportWalletError({
    hasError: !result.success,
    errorMessage: result.errorMessage ?? undefined
  }))
})

handler.on(WalletPageActions.importFromMetaMask.getType(), async (store: Store, payload: ImportFromExternalWalletPayloadType) => {
  const braveWalletService = getWalletPageApiProxy().braveWalletService
  const keyringController = getWalletPageApiProxy().keyringController
  const result =
    await braveWalletService.importFromExternalWallet(
      BraveWallet.ExternalWalletType.MetaMask, payload.password, payload.newPassword)
  if (result.success) {
    keyringController.notifyWalletBackupComplete()
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

// TODO(bbondy): Remove - Example usage:
//
// Interacting with the token registry
// const ercTokenRegistry = getWalletPageApiProxy().ercTokenRegistry
// const val1 = await ercTokenRegistry.getAllTokens()
// const val2 = await ercTokenRegistry.getTokenBySymbol('BAT')
// const val3 = await ercTokenRegistry.getTokenByContract('0x0D8775F648430679A709E98d2b0Cb6250d2887EF')
// const val4 = await ercTokenRegistry.getTokenByContract('dsaida')
// console.log('val1-4: ', val1, val2, val3, val4)
//
// Getting and setting network:
//
// import { Network } from '../../constants/types'
// const ethJsonRpcController = getWalletPageApiProxy().ethJsonRpcController
// const network = await ethJsonRpcController.getNetwork()
// await ethJsonRpcController.setNetwork(Network.Rinkeby)
// const chainId = await ethJsonRpcController.getChainId()
// const blockTrackerUrl = await ethJsonRpcController.getBlockTrackerUrl()
//
// Getting ETH and BAT ERC20 balance
// const balance = await ethJsonRpcController.getBalance(address)
// const token_balance = await ethJsonRpcController.getERC20TokenBalance('0x0d8775f648430679a709e98d2b0cb6250d2887ef', address)

export default handler.middleware
