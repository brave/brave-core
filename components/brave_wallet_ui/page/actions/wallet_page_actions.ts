/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import {
  CreateWalletPayloadType,
  WalletCreatedPayloadType,
  RecoveryWordsAvailablePayloadType,
  SelectAssetPayloadType,
  UpdateSelectedAssetType,
  ImportAccountPayloadType,
  RemoveImportedAccountPayloadType,
  RemoveHardwareAccountPayloadType,
  ImportAccountFromJsonPayloadType,
  ImportFromExternalWalletPayloadType,
  ImportWalletErrorPayloadType,
  ImportFilecoinAccountPayloadType,
  RestoreWalletPayloadType,
  ShowRecoveryPhrasePayload
} from '../constants/action_types'
import {
  BraveWallet,
  NFTMetadataReturnType,
  UpdateAccountNamePayloadType,
  ImportAccountErrorType
} from '../../constants/types'

export const createWallet = createAction<CreateWalletPayloadType>('createWallet')
export const restoreWallet = createAction<RestoreWalletPayloadType>('restoreWallet')
export const importAccount = createAction<ImportAccountPayloadType>('importAccount')
export const importFilecoinAccount = createAction<ImportFilecoinAccountPayloadType>('importFilecoinAccount')
export const importAccountFromJson = createAction<ImportAccountFromJsonPayloadType>('importAccountFromJson')
export const removeImportedAccount = createAction<RemoveImportedAccountPayloadType>('removeImportedAccount')
export const walletCreated = createAction<WalletCreatedPayloadType>('walletCreated')
export const walletSetupComplete = createAction<boolean>('walletSetupComplete')
export const showRecoveryPhrase = createAction<ShowRecoveryPhrasePayload>('showRecoveryPhrase')
export const recoveryWordsAvailable = createAction<RecoveryWordsAvailablePayloadType>('recoveryWordsAvailable')
export const walletBackupComplete = createAction('walletBackupComplete')
export const hasMnemonicError = createAction<boolean>('hasMnemonicError')
export const setShowAddModal = createAction<boolean>('setShowAddModal')
export const setImportAccountError = createAction<ImportAccountErrorType>('setImportAccountError')
export const setImportWalletError = createAction<ImportWalletErrorPayloadType>('setImportWalletError')
export const updatePriceInfo = createAction<SelectAssetPayloadType>('updatePriceInfo')
export const selectAsset = createAction<UpdateSelectedAssetType>('selectAsset')
export const updateSelectedAsset = createAction<BraveWallet.BlockchainToken | undefined>('updateSelectedAsset')
export const setIsFetchingPriceHistory = createAction<boolean>('setIsFetchingPriceHistory')
export const setShowIsRestoring = createAction<boolean>('setShowIsRestoring')
export const updateAccountName = createAction<UpdateAccountNamePayloadType>('updateAccountName')
export const addHardwareAccounts = createAction<BraveWallet.HardwareWalletAccount[]>('addHardwareAccounts')
export const removeHardwareAccount = createAction<RemoveHardwareAccountPayloadType>('removeHardwareAccount')
export const checkWalletsToImport = createAction('checkWalletsToImport')
export const setCryptoWalletsInitialized = createAction<boolean>('setCryptoWalletsInitialized')
export const setMetaMaskInitialized = createAction<boolean>('setMetaMaskInitialized')
export const setImportWalletsCheckComplete = createAction<boolean>('setImportWalletsCheckComplete')
export const importFromCryptoWallets = createAction<ImportFromExternalWalletPayloadType>('importFromCryptoWallets')
export const importFromMetaMask = createAction<ImportFromExternalWalletPayloadType>('importFromMetaMask')
export const openWalletSettings = createAction('openWalletSettings')
export const getNFTMetadata = createAction<BraveWallet.BlockchainToken>('getNFTMetadata')
export const setIsFetchingNFTMetadata = createAction<boolean>('setIsFetchingNFTMetadata')
export const updateNFTMetadata = createAction<NFTMetadataReturnType | undefined>('updateNFTMetadata')
export const updateNftMetadataError = createAction<string | undefined>('updateNftMetadataError')
export const onOnboardingShown = createAction('onOnboardingShown')
export const agreeToWalletTerms = createAction('agreeToWalletTerms')
export const selectCoinMarket = createAction<BraveWallet.CoinMarket | undefined>('selectCoinMarket')
