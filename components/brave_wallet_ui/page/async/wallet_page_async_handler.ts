// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, AnyAction } from 'redux'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletPageActions from '../actions/wallet_page_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import { UpdateAccountNamePayloadType, HardwareWalletAccount } from '../../constants/types'
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
  ImportFromExternalWalletPayloadType
} from '../constants/action_types'
import { NewUnapprovedTxAdded } from '../../common/constants/action_types'
import { fetchSwapQuoteFactory } from '../../common/async/wallet_async_handler'

type Store = MiddlewareAPI<Dispatch<AnyAction>, any>

const handler = new AsyncActionHandler()

async function getAPIProxy () {
  // TODO(petemill): don't lazy import() if this actually makes the time-to-first-data slower!
  const api = await import('../wallet_page_api_proxy.js')
  return api.default.getInstance()
}

async function refreshWalletInfo (store: Store) {
  const walletHandler = (await getAPIProxy()).walletHandler
  const result = await walletHandler.getWalletInfo()
  store.dispatch(WalletActions.initialized(result))
}

handler.on(WalletPageActions.createWallet.getType(), async (store, payload: CreateWalletPayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  const result = await keyringController.createWallet(payload.password)
  store.dispatch(WalletPageActions.walletCreated({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.restoreWallet.getType(), async (store, payload: RestoreWalletPayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  const result = await keyringController.restoreWallet(payload.mnemonic, payload.password, payload.isLegacy)
  if (!result.isValidMnemonic) {
    store.dispatch(WalletPageActions.hasMnemonicError(!result.isValidMnemonic))
    return
  }
  await keyringController.notifyWalletBackupComplete()
  await refreshWalletInfo(store)
  store.dispatch(WalletPageActions.setShowIsRestoring(false))
})

handler.on(WalletPageActions.addAccount.getType(), async (store, payload: AddAccountPayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  const result = await keyringController.addAccount(payload.accountName)
  return result.success
})

handler.on(WalletPageActions.showRecoveryPhrase.getType(), async (store, payload: boolean) => {
  const keyringController = (await getAPIProxy()).keyringController
  const result = await keyringController.getMnemonicForDefaultKeyring()
  store.dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.walletBackupComplete.getType(), async (store) => {
  const keyringController = (await getAPIProxy()).keyringController
  await keyringController.notifyWalletBackupComplete()
})

handler.on(WalletPageActions.selectAsset.getType(), async (store, payload: UpdateSelectedAssetType) => {
  store.dispatch(WalletPageActions.updateSelectedAsset(payload.asset))
  store.dispatch(WalletPageActions.setIsFetchingPriceHistory(true))
  const assetPriceController = (await getAPIProxy()).assetRatioController
  if (payload.asset) {
    const priceInfo = await assetPriceController.getPrice([payload.asset.symbol.toLowerCase()], ['usd', 'btc'], payload.timeFrame)
    const priceHistory = await assetPriceController.getPriceHistory(payload.asset.symbol.toLowerCase(), payload.timeFrame)
    store.dispatch(WalletPageActions.updatePriceInfo({ priceHistory: priceHistory, usdPriceInfo: priceInfo.values[0], btcPriceInfo: priceInfo.values[1], timeFrame: payload.timeFrame }))
  } else {
    store.dispatch(WalletPageActions.updatePriceInfo({ priceHistory: undefined, btcPriceInfo: undefined, usdPriceInfo: undefined, timeFrame: payload.timeFrame }))
  }
})

handler.on(WalletPageActions.importAccount.getType(), async (store, payload: ImportAccountPayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  const result = await keyringController.importAccount(payload.accountName, payload.privateKey)
  if (result.success) {
    store.dispatch(WalletPageActions.setImportError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportError(true))
  }
})

handler.on(WalletPageActions.importAccountFromJson.getType(), async (store, payload: ImportAccountFromJsonPayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  const result = await keyringController.importAccountFromJson(payload.accountName, payload.password, payload.json)
  if (result.success) {
    store.dispatch(WalletPageActions.setImportError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportError(true))
  }
})

handler.on(WalletPageActions.removeImportedAccount.getType(), async (store, payload: RemoveImportedAccountPayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  const result = await keyringController.removeImportedAccount(payload.address)
  return result.success
})

handler.on(WalletPageActions.viewPrivateKey.getType(), async (store, payload: ViewPrivateKeyPayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  const result = payload.isDefault ?
    await keyringController.getPrivateKeyForDefaultKeyringAccount(payload.address)
    : await keyringController.getPrivateKeyForImportedAccount(payload.address)
  store.dispatch(WalletPageActions.privateKeyAvailable({ privateKey: result.privateKey }))
})

handler.on(WalletPageActions.updateAccountName.getType(), async (store, payload: UpdateAccountNamePayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  const result = payload.isDerived ?
    await keyringController.setDefaultKeyringDerivedAccountName(payload.address, payload.name)
    : await keyringController.setDefaultKeyringImportedAccountName(payload.address, payload.name)
  return result.success
})

handler.on(WalletPageActions.addHardwareAccounts.getType(), async (store, accounts: HardwareWalletAccount[]) => {
  const keyringController = (await getAPIProxy()).keyringController
  await keyringController.addHardwareAccounts(accounts)
  store.dispatch(WalletPageActions.setShowAddModal(false))
})

handler.on(WalletPageActions.removeHardwareAccount.getType(), async (store, payload: RemoveHardwareAccountPayloadType) => {
  const keyringController = (await getAPIProxy()).keyringController
  await keyringController.removeHardwareAccount(payload.address)
  store.dispatch(WalletPageActions.setShowAddModal(false))
})

handler.on(WalletPageActions.checkWalletsToImport.getType(), async (store) => {
  const braveWalletService = (await getAPIProxy()).braveWalletService
  const cwResult = await braveWalletService.isCryptoWalletsInstalled()
  const mmResult = await braveWalletService.isMetaMaskInstalled()
  store.dispatch(WalletPageActions.setCryptoWalletsInstalled(cwResult.installed))
  store.dispatch(WalletPageActions.setMetaMaskInstalled(mmResult.installed))
})

handler.on(WalletPageActions.importFromCryptoWallets.getType(), async (store, payload: ImportFromExternalWalletPayloadType) => {
  const braveWalletService = (await getAPIProxy()).braveWalletService
  const result = await braveWalletService.importFromCryptoWallets(payload.password, payload.newPassword)
  store.dispatch(WalletPageActions.setImportError(!result.success))
})

handler.on(WalletPageActions.importFromMetaMask.getType(), async (store, payload: ImportFromExternalWalletPayloadType) => {
  const braveWalletService = (await getAPIProxy()).braveWalletService
  const result = await braveWalletService.importFromMetaMask(payload.password, payload.newPassword)
  store.dispatch(WalletPageActions.setImportError(!result.success))
})

handler.on(WalletActions.newUnapprovedTxAdded.getType(), async (store, payload: NewUnapprovedTxAdded) => {
  const pageHandler = (await getAPIProxy()).pageHandler
  await pageHandler.showApprovePanelUI()
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
// const ercTokenRegistry = (await getAPIProxy()).ercTokenRegistry
// const val1 = await ercTokenRegistry.getAllTokens()
// const val2 = await ercTokenRegistry.getTokenBySymbol('BAT')
// const val3 = await ercTokenRegistry.getTokenByContract('0x0D8775F648430679A709E98d2b0Cb6250d2887EF')
// const val4 = await ercTokenRegistry.getTokenByContract('dsaida')
// console.log('val1-4: ', val1, val2, val3, val4)
//
// Getting and setting network:
//
// import { Network } from '../../constants/types'
// const ethJsonRpcController = (await getAPIProxy()).ethJsonRpcController
// const network = await ethJsonRpcController.getNetwork()
// await ethJsonRpcController.setNetwork(Network.Rinkeby)
// const chainId = await ethJsonRpcController.getChainId()
// const blockTrackerUrl = await ethJsonRpcController.getBlockTrackerUrl()
//
// Getting ETH and BAT ERC20 balance
// const balance = await ethJsonRpcController.getBalance(address)
// const token_balance = await ethJsonRpcController.getERC20TokenBalance('0x0d8775f648430679a709e98d2b0cb6250d2887ef', address)

export default handler.middleware
