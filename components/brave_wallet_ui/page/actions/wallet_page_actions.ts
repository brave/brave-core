/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import {
  CreateWalletPayloadType,
  WalletCreatedPayloadType,
  RecoveryWordsAvailablePayloadType,
  PrivateKeyAvailablePayloadType,
  RestoreWalletPayloadType,
  SelectAssetPayloadType,
  UpdateSelectedAssetType,
  AddAccountPayloadType,
  ImportAccountPayloadType,
  RemoveImportedAccountPayloadType,
  ViewPrivateKeyPayloadType,
  ImportAccountFromJsonPayloadType
} from '../constants/action_types'
import { TokenInfo, UpdateAccountNamePayloadType } from '../../constants/types'

export const createWallet = createAction<CreateWalletPayloadType>('createWallet')
export const restoreWallet = createAction<RestoreWalletPayloadType>('restoreWallet')
export const addAccount = createAction<AddAccountPayloadType>('addAccount')
export const importAccount = createAction<ImportAccountPayloadType>('importAccount')
export const importAccountFromJson = createAction<ImportAccountFromJsonPayloadType>('importAccountFromJson')
export const removeImportedAccount = createAction<RemoveImportedAccountPayloadType>('removeImportedAccount')
export const walletCreated = createAction<WalletCreatedPayloadType>('walletCreated')
export const walletSetupComplete = createAction('walletSetupComplete')
export const showRecoveryPhrase = createAction<boolean>('showRecoveryPhrase')
export const viewPrivateKey = createAction<ViewPrivateKeyPayloadType>('viewPrivateKey')
export const doneViewingPrivateKey = createAction('doneViewingPrivateKey')
export const recoveryWordsAvailable = createAction<RecoveryWordsAvailablePayloadType>('recoveryWordsAvailable')
export const privateKeyAvailable = createAction<PrivateKeyAvailablePayloadType>('privateKeyAvailable')
export const walletBackupComplete = createAction('walletBackupComplete')
export const hasMnemonicError = createAction<boolean>('hasMnemonicError')
export const setShowAddModal = createAction<boolean>('setShowAddModal')
export const setImportError = createAction<boolean>('setImportError')
export const updatePriceInfo = createAction<SelectAssetPayloadType>('updatePriceInfo')
export const selectAsset = createAction<UpdateSelectedAssetType>('selectAsset')
export const updateSelectedAsset = createAction<TokenInfo>('updateSelectedAsset')
export const setIsFetchingPriceHistory = createAction<boolean>('setIsFetchingPriceHistory')
export const setShowIsRestoring = createAction<boolean>('setShowIsRestoring')
export const updateAccountName = createAction<UpdateAccountNamePayloadType>('updateAccountName')
