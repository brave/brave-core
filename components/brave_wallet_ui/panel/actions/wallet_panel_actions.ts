/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import {
  AccountPayloadType,
  AddSuggestTokenProcessedPayload,
  ShowConnectToSitePayload,
  EthereumChainPayload,
  EthereumChainRequestPayload,
  SignMessagePayload,
  SignMessageProcessedPayload,
  SignMessageHardwareProcessedPayload,
  SwitchEthereumChainProcessedPayload
} from '../constants/action_types'
import { BraveWallet, SwapErrorResponse } from '../../constants/types'
import { SwapParamsPayloadType } from '../../common/constants/action_types'
import { HardwareWalletResponseCodeType } from '../../common/hardware/types'

export const connectToSite = createAction<AccountPayloadType>('connectToSite')
export const cancelConnectToSite = createAction<AccountPayloadType>('cancelConnectToSite')
export const visibilityChanged = createAction<boolean>('visibilityChanged')
export const showConnectToSite = createAction<ShowConnectToSitePayload>('showConnectToSite')
export const addEthereumChain = createAction<EthereumChainPayload>('addEthereumChain')
export const addEthereumChainRequestCompleted = createAction<EthereumChainRequestPayload>('AddEthereumChainRequestCompleted')
export const switchEthereumChain = createAction<BraveWallet.SwitchChainRequest>('switchEthereumChain')
export const switchEthereumChainProcessed = createAction<SwitchEthereumChainProcessedPayload>('switchEthereumChainProcessed')
export const showApproveTransaction = createAction('showApproveTransaction')
export const showUnlock = createAction('showUnlock')
export const setupWallet = createAction('setupWallet')
export const expandWallet = createAction('expandWallet')
export const openWalletSettings = createAction('openWalletSettings')
export const openWalletApps = createAction('openWalletApps')
export const expandRestoreWallet = createAction('expandRestoreWallet')
export const expandWalletAccounts = createAction('expandWalletAccounts')
export const expandWalletAddAsset = createAction('expandWalletAddAsset')
export const navigateTo = createAction<string>('navigateTo')
export const navigateToMain = createAction('navigateToMain')
export const setPanelSwapQuote = createAction<BraveWallet.SwapResponse>('setPanelSwapQuote')
export const setPanelSwapError = createAction<SwapErrorResponse | undefined>('setPanelSwapError')
export const fetchPanelSwapQuote = createAction<SwapParamsPayloadType>('fetchPanelSwapQuote')
export const signMessage = createAction<SignMessagePayload[]>('signMessage')
export const signMessageProcessed = createAction<SignMessageProcessedPayload>('signMessageProcessed')
export const signMessageHardware = createAction<BraveWallet.SignMessageRequest>('signMessageHardware')
export const signMessageHardwareProcessed = createAction<SignMessageHardwareProcessedPayload>('signMessageHardwareProcessed')
export const approveHardwareTransaction = createAction<BraveWallet.TransactionInfo>('approveHardwareTransaction')
export const setHardwareWalletInteractionError = createAction<HardwareWalletResponseCodeType | undefined>('setHardwareWalletInteractionError')
export const cancelConnectHardwareWallet = createAction<BraveWallet.TransactionInfo>('cancelConnectHardwareWallet')
export const addSuggestToken = createAction<BraveWallet.AddSuggestTokenRequest>('addSuggestToken')
export const addSuggestTokenProcessed = createAction<AddSuggestTokenProcessedPayload>('addSuggestTokenProcessed')
export const setSelectedTransaction = createAction<BraveWallet.TransactionInfo | undefined>('setSelectedTransaction')
