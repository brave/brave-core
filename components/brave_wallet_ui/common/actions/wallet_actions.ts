/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import {
  InitializedPayloadType,
  UnlockWalletPayloadType,
  ChainChangedEventPayloadType,
  NewUnapprovedTxAdded,
  UnapprovedTxUpdated,
  TransactionStatusChanged,
  AddUserAssetPayloadType,
  SetUserAssetVisiblePayloadType,
  RemoveUserAssetPayloadType
} from '../constants/action_types'
import {
  AppObjectType,
  WalletAccountType,
  EthereumChain,
  GetAllNetworksList,
  GetAllTokensReturnInfo,
  TokenInfo,
  GetETHBalancesPriceReturnInfo,
  GetERC20TokenBalanceAndPriceReturnInfo,
  PortfolioTokenHistoryAndInfo,
  AssetPriceTimeframe,
  SendTransactionParams,
  ER20TransferParams,
  TransactionInfo,
  TransactionListInfo
} from '../../constants/types'

export const initialize = createAction('initialize')
export const initialized = createAction<InitializedPayloadType>('initialized')
export const lockWallet = createAction('lockWallet')
export const unlockWallet = createAction<UnlockWalletPayloadType>('unlockWallet')
export const addFavoriteApp = createAction<AppObjectType>('addFavoriteApp')
export const removeFavoriteApp = createAction<AppObjectType>('removeFavoriteApp')
export const hasIncorrectPassword = createAction<boolean>('hasIncorrectPassword')
export const addUserAsset = createAction<AddUserAssetPayloadType>('addUserAsset')
export const addUserAssetError = createAction<boolean>('addUserAssetError')
export const removeUserAsset = createAction<RemoveUserAssetPayloadType>('removeUserAsset')
export const setUserAssetVisible = createAction<SetUserAssetVisiblePayloadType>('setUserAssetVisible')
export const setVisibleTokensInfo = createAction<TokenInfo[]>('setVisibleTokensInfo')
export const selectAccount = createAction<WalletAccountType>('selectAccount')
export const selectNetwork = createAction<EthereumChain>('selectNetwork')
export const setNetwork = createAction<EthereumChain>('setNetwork')
export const getAllNetworks = createAction('getAllNetworks')
export const setAllNetworks = createAction<GetAllNetworksList>('getAllNetworks')
export const chainChangedEvent = createAction<ChainChangedEventPayloadType>('chainChangedEvent')
export const keyringCreated = createAction('keyringCreated')
export const keyringRestored = createAction('keyringRestored')
export const locked = createAction('locked')
export const unlocked = createAction('unlocked')
export const backedUp = createAction('backedUp')
export const accountsChanged = createAction('accountsChanged')
export const setAllTokensList = createAction<GetAllTokensReturnInfo>('setAllTokensList')
export const getAllTokensList = createAction('getAllTokensList')
export const ethBalancesUpdated = createAction<GetETHBalancesPriceReturnInfo>('ethBalancesUpdated')
export const tokenBalancesUpdated = createAction<GetERC20TokenBalanceAndPriceReturnInfo>('tokenBalancesUpdated')
export const portfolioPriceHistoryUpdated = createAction<PortfolioTokenHistoryAndInfo[][]>('portfolioPriceHistoryUpdated')
export const selectPortfolioTimeline = createAction<AssetPriceTimeframe>('selectPortfolioTimeline')
export const portfolioTimelineUpdated = createAction<AssetPriceTimeframe>('portfolioTimelineUpdated')
export const sendTransaction = createAction<SendTransactionParams>('sendTransaction')
export const sendERC20Transfer = createAction<ER20TransferParams>('sendERC20Transfer')
export const newUnapprovedTxAdded = createAction<NewUnapprovedTxAdded>('newUnapprovedTxAdded')
export const unapprovedTxUpdated = createAction<UnapprovedTxUpdated>('unapprovedTxUpdated')
export const transactionStatusChanged = createAction<TransactionStatusChanged>('transactionStatusChanged')
export const approveTransaction = createAction<TransactionInfo>('approveTransaction')
export const rejectTransaction = createAction<TransactionInfo>('rejectTransaction')
export const knownTransactionsUpdated = createAction<TransactionInfo[]>('knownTransactionsUpdated')
export const setTransactionList = createAction<TransactionListInfo[]>('setTransactionList')
