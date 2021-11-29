/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import {
  AccountPayloadType,
  ShowConnectToSitePayload,
  EthereumChainPayload,
  EthereumChainRequestPayload,
  SignMessagePayload,
  SignMessageProcessedPayload,
  SignMessageHardwareProcessedPayload,
  SwitchEthereumChainProcessedPayload
} from '../constants/action_types'
import {
  SwapErrorResponse,
  SwapResponse,
  SignMessageRequest,
  SwitchChainRequest,
  HardwareWalletResponseCodeType
} from '../../constants/types'
import { SwapParamsPayloadType } from '../../common/constants/action_types'
import { TransactionInfo } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'

export const connectToSite = createAction<AccountPayloadType>('connectToSite')
export const cancelConnectToSite = createAction<AccountPayloadType>('cancelConnectToSite')
export const visibilityChanged = createAction<boolean>('visibilityChanged')
export const showConnectToSite = createAction<ShowConnectToSitePayload>('showConnectToSite')
export const addEthereumChain = createAction<EthereumChainPayload>('addEthereumChain')
export const addEthereumChainRequestCompleted = createAction<EthereumChainRequestPayload>('AddEthereumChainRequestCompleted')
export const switchEthereumChain = createAction<SwitchChainRequest>('switchEthereumChain')
export const switchEthereumChainProcessed = createAction<SwitchEthereumChainProcessedPayload>('switchEthereumChainProcessed')
export const showApproveTransaction = createAction('showApproveTransaction')
export const setupWallet = createAction('setupWallet')
export const expandWallet = createAction('expandWallet')
export const openWalletSettings = createAction('openWalletSettings')
export const openWalletApps = createAction('openWalletApps')
export const expandRestoreWallet = createAction('expandRestoreWallet')
export const expandWalletAccounts = createAction('expandWalletAccounts')
export const navigateTo = createAction<string>('navigateTo')
export const navigateToMain = createAction('navigateToMain')
export const setPanelSwapQuote = createAction<SwapResponse>('setPanelSwapQuote')
export const setPanelSwapError = createAction<SwapErrorResponse | undefined>('setPanelSwapError')
export const fetchPanelSwapQuote = createAction<SwapParamsPayloadType>('fetchPanelSwapQuote')
export const signMessage = createAction<SignMessagePayload[]>('signMessage')
export const signMessageProcessed = createAction<SignMessageProcessedPayload>('signMessageProcessed')
export const signMessageHardware = createAction<SignMessageRequest>('signMessageHardware')
export const signMessageHardwareProcessed = createAction<SignMessageHardwareProcessedPayload>('signMessageHardwareProcessed')
export const approveHardwareTransaction = createAction<TransactionInfo>('approveHardwareTransaction')
export const setHardwareWalletInteractionError = createAction<HardwareWalletResponseCodeType | undefined>('setHardwareWalletInteractionError')
export const cancelConnectHardwareWallet = createAction<TransactionInfo>('cancelConnectHardwareWallet')
