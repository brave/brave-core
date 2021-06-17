/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import { CreateWalletPayloadType, WalletCreatedPayloadType, RecoveryWordsAvailablePayloadType, RestoreWalletPayloadType } from '../constants/action_types'
import { ChartTimelineType, AssetOptionType } from '../../constants/types'

export const createWallet = createAction<CreateWalletPayloadType>('createWallet')
export const restoreWallet = createAction<RestoreWalletPayloadType>('restoreWallet')
export const walletCreated = createAction<WalletCreatedPayloadType>('walletCreated')
export const walletSetupComplete = createAction('walletSetupComplete')
export const showRecoveryPhrase = createAction<boolean>('showRecoveryPhrase')
export const recoveryWordsAvailable = createAction<RecoveryWordsAvailablePayloadType>('recoveryWordsAvailable')
export const walletBackupComplete = createAction('walletBackupComplete')
export const hasMnemonicError = createAction<boolean>('hasMnemonicError')
export const changeTimline = createAction<ChartTimelineType>('changeTimline')
export const selectAsset = createAction<AssetOptionType>('selectAsset')
