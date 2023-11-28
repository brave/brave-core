/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createAction } from '@reduxjs/toolkit'
import {
  GetEncryptionPublicKeyProcessedPayload,
  DecryptProcessedPayload,
  ShowConnectToSitePayload,
  EthereumChainRequestPayload,
  SignMessageProcessedPayload,
  SignAllTransactionsProcessedPayload,
  SwitchEthereumChainProcessedPayload,
  SignTransactionHardwarePayload,
  SignAllTransactionsHardwarePayload,
  SignMessageHardwarePayload
} from '../constants/action_types'
import { BraveWallet, PanelTypes } from '../../constants/types'
import { HardwareWalletResponseCodeType } from '../../common/hardware/types'

export const visibilityChanged = createAction<boolean>('visibilityChanged')
export const showConnectToSite =
  createAction<ShowConnectToSitePayload>('showConnectToSite')
export const addEthereumChain =
  createAction<BraveWallet.AddChainRequest>('addEthereumChain')
export const addEthereumChainRequestCompleted =
  createAction<EthereumChainRequestPayload>('addEthereumChainRequestCompleted')
export const switchEthereumChain = createAction<BraveWallet.SwitchChainRequest>(
  'switchEthereumChain'
)
export const switchEthereumChainProcessed =
  createAction<SwitchEthereumChainProcessedPayload>(
    'switchEthereumChainProcessed'
  )
export const showUnlock = createAction('showUnlock')
export const setupWallet = createAction('setupWallet')
export const openWalletSettings = createAction('openWalletSettings')
export const navigateTo = createAction<PanelTypes>('navigateTo')
export const navigateToMain = createAction('navigateToMain')
export const signMessage =
  createAction<BraveWallet.SignMessageRequest[]>('signMessage')
export const signMessageProcessed = createAction<SignMessageProcessedPayload>(
  'signMessageProcessed'
)
export const signMessageHardware = createAction<SignMessageHardwarePayload>(
  'signMessageHardware'
)
export const signMessageHardwareProcessed =
  createAction<SignMessageProcessedPayload>('signMessageHardwareProcessed')
export const setHardwareWalletInteractionError = createAction<
  HardwareWalletResponseCodeType | undefined
>('setHardwareWalletInteractionError')
export const cancelConnectHardwareWallet =
  createAction<BraveWallet.AccountInfo>('cancelConnectHardwareWallet')
export const getEncryptionPublicKey =
  createAction<BraveWallet.GetEncryptionPublicKeyRequest>(
    'getEncryptionPublicKey'
  )
export const getEncryptionPublicKeyProcessed =
  createAction<GetEncryptionPublicKeyProcessedPayload>(
    'getEncryptionPublicKeyProcessed'
  )
export const decrypt = createAction<BraveWallet.DecryptRequest>('decrypt')
export const decryptProcessed =
  createAction<DecryptProcessedPayload>('decryptProcessed')
export const setSelectedTransactionId = createAction<string | undefined>(
  'setSelectedTransactionId'
)
export const signTransaction =
  createAction<BraveWallet.SignTransactionRequest[]>('signTransaction')
export const signTransactionHardware =
  createAction<SignTransactionHardwarePayload>('signTransactionHardware')
export const signTransactionProcessed =
  createAction<SignMessageProcessedPayload>('signTransactionProcessed')
export const signAllTransactions = createAction<
  BraveWallet.SignAllTransactionsRequest[]
>('signAllTransactions')
export const signAllTransactionsHardware =
  createAction<SignAllTransactionsHardwarePayload>(
    'signAllTransactionsHardware'
  )
export const signAllTransactionsProcessed =
  createAction<SignAllTransactionsProcessedPayload>(
    'signAllTransactionsProcessed'
  )
export const setCloseOnDeactivate = createAction<boolean>(
  'setCloseOnDeactivate'
)
export const signMessageError =
  createAction<BraveWallet.SignMessageError[]>('signMessageError')
export const signMessageErrorProcessed = createAction<string>(
  'signMessageErrorProcessed'
)
