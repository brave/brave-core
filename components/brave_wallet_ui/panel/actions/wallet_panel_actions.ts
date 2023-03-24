/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createAction } from '@reduxjs/toolkit'
import {
  ConnectWithSitePayloadType,
  AddSuggestTokenProcessedPayload,
  CancelConnectHardwareWalletPayload,
  GetEncryptionPublicKeyProcessedPayload,
  DecryptProcessedPayload,
  ShowConnectToSitePayload,
  EthereumChainRequestPayload,
  // SignMessagePayload,
  SignMessageProcessedPayload,
  SignAllTransactionsProcessedPayload,
  SwitchEthereumChainProcessedPayload
} from '../constants/action_types'
import { BraveWallet, PanelTypes, SerializableDecryptRequest, SerializableGetEncryptionPublicKeyRequest, SerializableSignMessageRequest, SerializableSwitchChainRequest, SerializableTransactionInfo } from '../../constants/types'
import { HardwareWalletResponseCodeType } from '../../common/hardware/types'

export const connectToSite = createAction<ConnectWithSitePayloadType>('connectToSite')
export const cancelConnectToSite = createAction('cancelConnectToSite')
export const visibilityChanged = createAction<boolean>('visibilityChanged')
export const showConnectToSite = createAction<ShowConnectToSitePayload>('showConnectToSite')
export const addEthereumChain = createAction<BraveWallet.AddChainRequest>('addEthereumChain')
export const addEthereumChainRequestCompleted = createAction<EthereumChainRequestPayload>('addEthereumChainRequestCompleted')
export const switchEthereumChain = createAction<SerializableSwitchChainRequest>('switchEthereumChain')
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
export const navigateTo = createAction<PanelTypes>('navigateTo')
export const navigateToMain = createAction('navigateToMain')
export const navigateBack = createAction('navigateBack')
export const signMessage = createAction<SerializableSignMessageRequest[]>('signMessage')
export const signMessageProcessed = createAction<SignMessageProcessedPayload>('signMessageProcessed')
export const signMessageHardware = createAction<SerializableSignMessageRequest>('signMessageHardware')
export const signMessageHardwareProcessed = createAction<SignMessageProcessedPayload>('signMessageHardwareProcessed')
export const approveHardwareTransaction = createAction<SerializableTransactionInfo>('approveHardwareTransaction')
export const setHardwareWalletInteractionError = createAction<HardwareWalletResponseCodeType | undefined>('setHardwareWalletInteractionError')
export const cancelConnectHardwareWallet = createAction<CancelConnectHardwareWalletPayload>('cancelConnectHardwareWallet')
export const addSuggestToken = createAction<BraveWallet.AddSuggestTokenRequest>('addSuggestToken')
export const addSuggestTokenProcessed = createAction<AddSuggestTokenProcessedPayload>('addSuggestTokenProcessed')
export const getEncryptionPublicKey = createAction<SerializableGetEncryptionPublicKeyRequest>('getEncryptionPublicKey')
export const getEncryptionPublicKeyProcessed = createAction<GetEncryptionPublicKeyProcessedPayload>('getEncryptionPublicKeyProcessed')
export const decrypt = createAction<SerializableDecryptRequest>('decrypt')
export const decryptProcessed = createAction<DecryptProcessedPayload>('decryptProcessed')
export const setSelectedTransaction = createAction<SerializableTransactionInfo | undefined>('setSelectedTransaction')
export const signTransaction = createAction<BraveWallet.SignTransactionRequest[]>('signTransaction')
export const signTransactionHardware = createAction<BraveWallet.SignTransactionRequest>('signTransactionHardware')
export const signTransactionProcessed = createAction<SignMessageProcessedPayload>('signTransactionProcessed')
export const signAllTransactions = createAction<BraveWallet.SignAllTransactionsRequest[]>('signAllTransactions')
export const signAllTransactionsHardware = createAction<BraveWallet.SignAllTransactionsRequest>('signAllTransactionsHardware')
export const signAllTransactionsProcessed = createAction<SignAllTransactionsProcessedPayload>('signAllTransactionsProcessed')
