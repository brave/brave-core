/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import { InitializedPayloadType, UnlockWalletPayloadType, SetInitialAccountNamesPayloadType, AddNewAccountNamePayloadType, ChainChangedEventPayloadType } from '../constants/action_types'
import { AppObjectType, WalletAccountType, Network } from '../../constants/types'

export const initialize = createAction('initialize')
export const initialized = createAction<InitializedPayloadType>('initialized')
export const lockWallet = createAction('lockWallet')
export const unlockWallet = createAction<UnlockWalletPayloadType>('unlockWallet')
export const addFavoriteApp = createAction<AppObjectType>('addFavoriteApp')
export const removeFavoriteApp = createAction<AppObjectType>('removeFavoriteApp')
export const hasIncorrectPassword = createAction<boolean>('hasIncorrectPassword')
export const setInitialAccountNames = createAction<SetInitialAccountNamesPayloadType>('setInitialAccountNames')
export const addNewAccountName = createAction<AddNewAccountNamePayloadType>('addNewAccountName')
export const selectAccount = createAction<WalletAccountType>('selectAccount')
export const selectNetwork = createAction<Network>('selectNetwork')
export const setNetwork = createAction<Network>('setNetwork')
export const chainChangedEvent = createAction<ChainChangedEventPayloadType>('chainChangedEvent')
export const keyringCreated = createAction('keyringCreated')
export const keyringRestored = createAction('keyringRestored')
export const locked = createAction('locked')
export const unlocked = createAction('unlocked')
export const backedUp = createAction('backedUp')
export const accountsChanged = createAction('accountsChanged')
