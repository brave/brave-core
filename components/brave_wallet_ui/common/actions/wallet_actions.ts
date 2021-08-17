/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import {
  InitializedPayloadType,
  UnlockWalletPayloadType,
  ChainChangedEventPayloadType,
  SetInitialVisibleTokensPayloadType,
  NewUnapprovedTxAdded,
  TransactionStatusChanged
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
  SendTransactionParam
} from '../../constants/types'

export const initialize = createAction('initialize')
export const initialized = createAction<InitializedPayloadType>('initialized')
export const lockWallet = createAction('lockWallet')
export const unlockWallet = createAction<UnlockWalletPayloadType>('unlockWallet')
export const addFavoriteApp = createAction<AppObjectType>('addFavoriteApp')
export const removeFavoriteApp = createAction<AppObjectType>('removeFavoriteApp')
export const hasIncorrectPassword = createAction<boolean>('hasIncorrectPassword')
export const setInitialVisibleTokens = createAction<SetInitialVisibleTokensPayloadType>('setInitialVisibleTokens')
export const updateVisibleTokens = createAction<string[]>('updateVisibleTokens')
export const setVisibleTokens = createAction<string[]>('setVisibleTokens')
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
export const sendTransaction = createAction<SendTransactionParam>('sendTransaction')
export const newUnapprovedTxAdded = createAction<NewUnapprovedTxAdded>('newUnapprovedTxAdded')
export const transactionStatusChanged = createAction<TransactionStatusChanged>('transactionStatusChanged')
