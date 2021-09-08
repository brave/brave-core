/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import { AccountPayloadType, ShowConnectToSitePayload, EthereumChainPayload, EthereumChainRequestPayload } from '../constants/action_types'

export const connectToSite = createAction<AccountPayloadType>('connectToSite')
export const cancelConnectToSite = createAction<AccountPayloadType>('cancelConnectToSite')
export const visibilityChanged = createAction<boolean>('visibilityChanged')
export const showConnectToSite = createAction<ShowConnectToSitePayload>('showConnectToSite')
export const addEthereumChain = createAction<EthereumChainPayload>('addEthereumChain')
export const addEthereumChainRequestCompleted = createAction<EthereumChainRequestPayload>('AddEthereumChainRequestCompleted')
export const setupWallet = createAction('setupWallet')
export const expandWallet = createAction('expandWallet')
export const openWalletSettings = createAction('openWalletSettings')
export const openWalletApps = createAction('openWalletApps')
export const restoreWallet = createAction('restoreWallet')
export const navigateTo = createAction<string>('navigateTo')
