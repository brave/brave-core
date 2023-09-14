// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import getWalletPageApiProxy from '../wallet_page_api_proxy'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletPageActions from '../actions/wallet_page_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import {
  BraveWallet
} from '../../constants/types'
import {
  CreateWalletPayloadType,
  UpdateSelectedAssetType,
  ImportFromExternalWalletPayloadType,
  RestoreWalletPayloadType,
  ImportWalletErrorPayloadType,
  ShowRecoveryPhrasePayload
} from '../constants/action_types'
import { Store } from '../../common/async/types'
import { getLocale } from '../../../common/locale'

const handler = new AsyncActionHandler()

async function refreshWalletInfo(store: Store) {
  const proxy = getWalletPageApiProxy()
  const { walletInfo } = await proxy.walletHandler.getWalletInfo()
  const { allAccounts } = await proxy.keyringService.getAllAccounts()
  store.dispatch(WalletActions.initialized({ walletInfo, allAccounts }))
  store.dispatch(WalletActions.refreshAll({}))
}

async function importFromExternalWallet (
  walletType: BraveWallet.ExternalWalletType,
  payload: ImportFromExternalWalletPayloadType
): Promise<ImportWalletErrorPayloadType> {
  const {
    braveWalletService,
    keyringService
  } = getWalletPageApiProxy()

  const result = await braveWalletService.importFromExternalWallet(
    walletType,
    payload.password,
    payload.newPassword
  )

  // complete backup if a new password was provided
  if (payload.newPassword && result.success) {
    keyringService.notifyWalletBackupComplete()
  }

  // was the provided import password correct?
  const checkExistingPasswordError = result.errorMessage === getLocale('braveWalletImportPasswordError')
    ? result.errorMessage
    : undefined

  // was import successful (if attempted)
  const importError = payload.newPassword ? result.errorMessage || undefined : undefined

  return {
    hasError: !!(importError || checkExistingPasswordError),
    errorMessage: importError || checkExistingPasswordError,
    incrementAttempts: true
  }
}

handler.on(WalletPageActions.createWallet.type, async (store: Store, payload: CreateWalletPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.createWallet(payload.password)
  store.dispatch(WalletPageActions.walletCreated({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.restoreWallet.type, async (store: Store, payload: RestoreWalletPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.restoreWallet(payload.mnemonic, payload.password, payload.isLegacy)
  if (!result.isValidMnemonic) {
    store.dispatch(WalletPageActions.hasMnemonicError(!result.isValidMnemonic))
    return
  }
  keyringService.notifyWalletBackupComplete()
  await refreshWalletInfo(store)
  store.dispatch(WalletPageActions.setShowIsRestoring(false))
  if (payload?.completeWalletSetup) {
    store.dispatch(WalletPageActions.walletSetupComplete(payload.completeWalletSetup))
  }
})

handler.on(WalletPageActions.showRecoveryPhrase.type, async (store: Store, {
  password,
  show
}: ShowRecoveryPhrasePayload) => {
  if (password) {
    const { keyringService } = getWalletPageApiProxy()
    const { mnemonic } = await keyringService.getMnemonicForDefaultKeyring(password)
    store.dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic }))
    return
  }

  store.dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: '' }))
})

handler.on(WalletPageActions.walletBackupComplete.type, async (store) => {
  const keyringService = getWalletPageApiProxy().keyringService
  keyringService.notifyWalletBackupComplete()
})

handler.on(WalletPageActions.selectAsset.type, async (store: Store, payload: UpdateSelectedAssetType) => {
  store.dispatch(WalletPageActions.updateSelectedAsset(payload.asset))
  if (payload.asset) {
    store.dispatch(WalletPageActions.selectPriceTimeframe(payload.timeFrame))
  }
})

handler.on(WalletPageActions.addHardwareAccounts.type, async (store: Store, accounts: BraveWallet.HardwareWalletAccount[]) => {
  const keyringService = getWalletPageApiProxy().keyringService
  keyringService.addHardwareAccounts(accounts)
})

handler.on(WalletPageActions.checkWalletsToImport.type, async (store) => {
  store.dispatch(WalletPageActions.setImportWalletsCheckComplete(false))
  const braveWalletService = getWalletPageApiProxy().braveWalletService
  const cwResult =
    await braveWalletService.isExternalWalletInitialized(
      BraveWallet.ExternalWalletType.CryptoWallets)
  const mmResult =
    await braveWalletService.isExternalWalletInitialized(
      BraveWallet.ExternalWalletType.MetaMask)
  store.dispatch(WalletPageActions.setCryptoWalletsInitialized(cwResult.initialized))
  store.dispatch(WalletPageActions.setMetaMaskInitialized(mmResult.initialized))
  store.dispatch(WalletPageActions.setImportWalletsCheckComplete(true))
})

handler.on(WalletPageActions.importFromCryptoWallets.type, async (store: Store, payload: ImportFromExternalWalletPayloadType) => {
  const results: ImportWalletErrorPayloadType = await importFromExternalWallet(
    BraveWallet.ExternalWalletType.CryptoWallets,
    payload
  )
  store.dispatch(WalletPageActions.setImportWalletError(results))
})

handler.on(WalletPageActions.importFromMetaMask.type, async (store: Store, payload: ImportFromExternalWalletPayloadType) => {
  const results: ImportWalletErrorPayloadType = await importFromExternalWallet(
    BraveWallet.ExternalWalletType.MetaMask,
    payload
  )
  store.dispatch(WalletPageActions.setImportWalletError(results))
})

handler.on(WalletPageActions.openWalletSettings.type, async (store) => {
  chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

export default handler.middleware
