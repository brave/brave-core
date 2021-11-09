/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import {
  UnlockWalletPayloadType,
  ChainChangedEventPayloadType,
  IsEip1559Changed,
  NewUnapprovedTxAdded,
  UnapprovedTxUpdated,
  TransactionStatusChanged,
  ActiveOriginChanged,
  DefaultWalletChanged,
  DefaultBaseCurrencyChanged,
  DefaultBaseCryptocurrencyChanged,
  AddUserAssetPayloadType,
  SetUserAssetVisiblePayloadType,
  RemoveUserAssetPayloadType,
  UpdateUnapprovedTransactionGasFieldsType,
  SitePermissionsPayloadType,
  RemoveSitePermissionPayloadType,
  UpdateUnapprovedTransactionSpendAllowanceType
} from '../constants/action_types'
import {
  AppItem,
  WalletAccountType,
  EthereumChain,
  GetAllNetworksList,
  GetAllTokensReturnInfo,
  ERCToken,
  GetNativeAssetBalancesPriceReturnInfo,
  GetERC20TokenBalanceAndPriceReturnInfo,
  PortfolioTokenHistoryAndInfo,
  AssetPriceTimeframe,
  SendTransactionParams,
  ER20TransferParams,
  ERC721TransferFromParams,
  TransactionInfo,
  AccountTransactions,
  DefaultWallet,
  GasEstimation1559,
  ApproveERC20Params,
  WalletInfoBase,
  WalletInfo
} from '../../constants/types'

export const initialize = createAction('initialize')
export const initialized = createAction<WalletInfo>('initialized')
export const lockWallet = createAction('lockWallet')
export const unlockWallet = createAction<UnlockWalletPayloadType>('unlockWallet')
export const addFavoriteApp = createAction<AppItem>('addFavoriteApp')
export const removeFavoriteApp = createAction<AppItem>('removeFavoriteApp')
export const hasIncorrectPassword = createAction<boolean>('hasIncorrectPassword')
export const addUserAsset = createAction<AddUserAssetPayloadType>('addUserAsset')
export const addUserAssetError = createAction<boolean>('addUserAssetError')
export const removeUserAsset = createAction<RemoveUserAssetPayloadType>('removeUserAsset')
export const setUserAssetVisible = createAction<SetUserAssetVisiblePayloadType>('setUserAssetVisible')
export const setVisibleTokensInfo = createAction<ERCToken[]>('setVisibleTokensInfo')
export const selectAccount = createAction<WalletAccountType>('selectAccount')
export const selectNetwork = createAction<EthereumChain>('selectNetwork')
export const setNetwork = createAction<EthereumChain>('setNetwork')
export const getAllNetworks = createAction('getAllNetworks')
export const setAllNetworks = createAction<GetAllNetworksList>('getAllNetworks')
export const chainChangedEvent = createAction<ChainChangedEventPayloadType>('chainChangedEvent')
export const isEip1559Changed = createAction<IsEip1559Changed>('isEip1559Changed')
export const keyringCreated = createAction('keyringCreated')
export const keyringRestored = createAction('keyringRestored')
export const locked = createAction('locked')
export const unlocked = createAction('unlocked')
export const backedUp = createAction('backedUp')
export const accountsChanged = createAction('accountsChanged')
export const selectedAccountChanged = createAction('selectedAccountChanged')
export const setAllTokensList = createAction<GetAllTokensReturnInfo>('setAllTokensList')
export const getAllTokensList = createAction('getAllTokensList')
export const nativeAssetBalancesUpdated = createAction<GetNativeAssetBalancesPriceReturnInfo>('nativeAssetBalancesUpdated')
export const tokenBalancesUpdated = createAction<GetERC20TokenBalanceAndPriceReturnInfo>('tokenBalancesUpdated')
export const portfolioPriceHistoryUpdated = createAction<PortfolioTokenHistoryAndInfo[][]>('portfolioPriceHistoryUpdated')
export const selectPortfolioTimeline = createAction<AssetPriceTimeframe>('selectPortfolioTimeline')
export const portfolioTimelineUpdated = createAction<AssetPriceTimeframe>('portfolioTimelineUpdated')
export const sendTransaction = createAction<SendTransactionParams>('sendTransaction')
export const sendERC20Transfer = createAction<ER20TransferParams>('sendERC20Transfer')
export const sendERC721TransferFrom = createAction<ERC721TransferFromParams>('sendERC721TransferFrom')
export const approveERC20Allowance = createAction<ApproveERC20Params>('approveERC20Allowance')
export const newUnapprovedTxAdded = createAction<NewUnapprovedTxAdded>('newUnapprovedTxAdded')
export const unapprovedTxUpdated = createAction<UnapprovedTxUpdated>('unapprovedTxUpdated')
export const transactionStatusChanged = createAction<TransactionStatusChanged>('transactionStatusChanged')
export const approveTransaction = createAction<TransactionInfo>('approveTransaction')
export const rejectTransaction = createAction<TransactionInfo>('rejectTransaction')
export const rejectAllTransactions = createAction('rejectAllTransactions')
export const setAccountTransactions = createAction<AccountTransactions>('setAccountTransactions')
export const defaultWalletUpdated = createAction<DefaultWallet>('defaultWalletUpdated')
export const setSelectedAccount = createAction<WalletAccountType>('setSelectedAccount')
export const activeOriginChanged = createAction<ActiveOriginChanged>('activeOriginChanged')
export const refreshGasEstimates = createAction('refreshGasEstimates')
export const setGasEstimates = createAction<GasEstimation1559>('setGasEstimates')
export const updateUnapprovedTransactionGasFields = createAction<UpdateUnapprovedTransactionGasFieldsType>('updateUnapprovedTransactionGasFields')
export const updateUnapprovedTransactionSpendAllowance = createAction<UpdateUnapprovedTransactionSpendAllowanceType>('updateUnapprovedTransactionSpendAllowance')
export const defaultWalletChanged = createAction<DefaultWalletChanged>('defaultWalletChanged')
export const defaultBaseCurrencyChanged = createAction<DefaultBaseCurrencyChanged>('defaultBaseCurrencyChanged')
export const defaultBaseCryptocurrencyChanged = createAction<DefaultBaseCryptocurrencyChanged>('defaultBaseCryptocurrencyChanged')
export const setSitePermissions = createAction<SitePermissionsPayloadType>('setSitePermissions')
export const removeSitePermission = createAction<RemoveSitePermissionPayloadType>('removeSitePermission')
export const queueNextTransaction = createAction('queueNextTransaction')
export const refreshBalancesAndPrices = createAction('refreshBalancesAndPrices')
export const setMetaMaskInstalled = createAction<boolean>('setMetaMaskInstalled')
export const refreshAccountInfo = createAction<WalletInfoBase>('refreshAccountInfo')
export const autoLockMinutesChanged = createAction('autoLockMinutesChanged')
export const retryTransaction = createAction<TransactionInfo>('retryTransaction')
export const cancelTransaction = createAction<TransactionInfo>('cancelTransaction')
export const speedupTransaction = createAction<TransactionInfo>('speedupTransaction')
