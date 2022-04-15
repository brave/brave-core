/* Copyright (c) 2022 The Brave Authors. All rights reserved.
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
  DefaultWalletChanged,
  DefaultBaseCurrencyChanged,
  DefaultBaseCryptocurrencyChanged,
  SetUserAssetVisiblePayloadType,
  UpdateUnapprovedTransactionGasFieldsType,
  SitePermissionsPayloadType,
  RemoveSitePermissionPayloadType,
  AddSitePermissionPayloadType,
  UpdateUnapprovedTransactionSpendAllowanceType,
  UpdateUnapprovedTransactionNonceType,
  SetTransactionProviderErrorType,
  SetAssetBalancesPayload,
  BalancesAndPricesRefreshedPayload,
  WalletInfoUpdatedPayload,
  ChainChangeCompletedPayload,
  SelectAccountCompletedPayload,
  AccountUpdatedPayload,
  WalletDataInitializedPayload,
  BalancesAndPricesHistoryRefreshedPayload,
  RefreshTokenPriceHistoryResult,
  TransactionHistoryRefreshedPayload
} from '../constants/action_types'
import {
  BraveWallet,
  WalletAccountType,
  GetNativeAssetBalancesPayload,
  GetBlockchainTokenBalanceReturnInfo,
  SendTransactionParams,
  ER20TransferParams,
  ERC721TransferFromParams,
  AccountTransactions,
  ApproveERC20Params,
  WalletInfoBase,
  WalletInfo,
  DefaultCurrencies,
  GetPriceReturnInfo,
  SolFeeEstimates,
  SPLTransferFromParams
} from '../../constants/types'

// bootup
export const initialize = createAction('initialize')
export const initialized = createAction<WalletInfo>('initialized')
export const ready = createAction('ready')

// lock - unlock
export const lockWallet = createAction('lockWallet')
export const locked = createAction('locked')
export const unlockWallet = createAction<UnlockWalletPayloadType>('unlockWallet')
export const unlocked = createAction('unlocked')
export const hasIncorrectPassword = createAction<boolean>('hasIncorrectPassword')
export const autoLockMinutesChanged = createAction('autoLockMinutesChanged')

// keyring
export const keyringCreated = createAction('keyringCreated')
export const keyringReset = createAction('keyringReset')
export const keyringRestored = createAction('keyringRestored')

// favorites
export const addFavoriteApp = createAction<BraveWallet.AppItem>('addFavoriteApp')
export const removeFavoriteApp = createAction<BraveWallet.AppItem>('removeFavoriteApp')

// assets
export const addUserAsset = createAction<BraveWallet.BlockchainToken>('addUserAsset')
export const addUserAssetError = createAction<boolean>('addUserAssetError')
export const removeUserAsset = createAction<BraveWallet.BlockchainToken>('removeUserAsset')
export const setUserAssetVisible = createAction<SetUserAssetVisiblePayloadType>('setUserAssetVisible')
export const setVisibleTokensInfo = createAction<BraveWallet.BlockchainToken[]>('setVisibleTokensInfo')
export const setAllTokensList = createAction<BraveWallet.BlockchainToken[]>('setAllTokensList')
export const getAllTokensList = createAction('getAllTokensList')
export const nativeAssetBalancesUpdated = createAction<GetNativeAssetBalancesPayload>('nativeAssetBalancesUpdated')
export const tokenBalancesUpdated = createAction<GetBlockchainTokenBalanceReturnInfo>('tokenBalancesUpdated')
export const setAssetBalances = createAction<SetAssetBalancesPayload>('assetBalancesUpdated')
export const setSelectedCoin = createAction<BraveWallet.CoinType>('setSelectedCoin')

// accounts
export const accountsChanged = createAction('accountsChanged')
export const selectAccount = createAction<WalletAccountType>('selectAccount')
export const selectedAccountChanged = createAction('selectedAccountChanged')
export const setAccount = createAction<SelectAccountCompletedPayload>('setAccount')
export const setDefaultAccounts = createAction<BraveWallet.AccountInfo[]>('setDefaultAccounts')
export const accountsUpdated = createAction<AccountUpdatedPayload>('accountsUpdated')
export const setSelectedAccount = createAction<WalletAccountType>('setSelectedAccount')
export const refreshAccountInfo = createAction<WalletInfoBase>('refreshAccountInfo')

// wallets
export const setMetaMaskInstalled = createAction<boolean>('setMetaMaskInstalled')
export const defaultWalletUpdated = createAction<BraveWallet.DefaultWallet>('defaultWalletUpdated')
export const defaultWalletChanged = createAction<DefaultWalletChanged>('defaultWalletChanged')

// backup
export const backedUp = createAction('backedUp')

// networks (chains)
export const selectNetwork = createAction<BraveWallet.NetworkInfo>('selectNetwork')
export const setNetwork = createAction<BraveWallet.NetworkInfo>('setNetwork')
export const getAllNetworks = createAction('getAllNetworks')
export const setAllNetworks = createAction<BraveWallet.NetworkInfo[]>('getAllNetworks')
export const chainChangedEvent = createAction<ChainChangedEventPayloadType>('chainChangedEvent')
export const chainChangeComplete = createAction<ChainChangeCompletedPayload>('chainChangeComplete')
export const isEip1559Changed = createAction<IsEip1559Changed>('isEip1559Changed')
export const expandWalletNetworks = createAction('expandWalletNetworks')
export const setDefaultNetworks = createAction<BraveWallet.NetworkInfo[]>('setDefaultNetworks')
export const setSelectedNetworkFilter = createAction<BraveWallet.NetworkInfo>('setSelectedNetworkFilter')

// prices
export const pricesUpdated = createAction<GetPriceReturnInfo>('pricesUpdated')
export const portfolioPriceHistoryUpdated = createAction<RefreshTokenPriceHistoryResult>('portfolioPriceHistoryUpdated')
export const selectPortfolioTimeline = createAction<BraveWallet.AssetPriceTimeframe>('selectPortfolioTimeline')
export const portfolioTimelineUpdated = createAction<BraveWallet.AssetPriceTimeframe>('portfolioTimelineUpdated')

// default currencies
export const defaultBaseCurrencyChanged = createAction<DefaultBaseCurrencyChanged>('defaultBaseCurrencyChanged')
export const defaultBaseCryptocurrencyChanged = createAction<DefaultBaseCryptocurrencyChanged>('defaultBaseCryptocurrencyChanged')
export const defaultCurrenciesUpdated = createAction<DefaultCurrencies>('defaultCurrenciesUpdated')

// transaction interactions
export const sendTransaction = createAction<SendTransactionParams>('sendTransaction')
export const sendERC20Transfer = createAction<ER20TransferParams>('sendERC20Transfer')
export const sendSPLTransfer = createAction<SPLTransferFromParams>('sendSPLTransfer')
export const sendERC721TransferFrom = createAction<ERC721TransferFromParams>('sendERC721TransferFrom')
export const approveERC20Allowance = createAction<ApproveERC20Params>('approveERC20Allowance')
export const newUnapprovedTxAdded = createAction<NewUnapprovedTxAdded>('newUnapprovedTxAdded')
export const unapprovedTxUpdated = createAction<UnapprovedTxUpdated>('unapprovedTxUpdated')
export const approveTransaction = createAction<BraveWallet.TransactionInfo>('approveTransaction')
export const rejectTransaction = createAction<BraveWallet.TransactionInfo>('rejectTransaction')
export const rejectAllTransactions = createAction('rejectAllTransactions')
export const retryTransaction = createAction<BraveWallet.TransactionInfo>('retryTransaction')
export const cancelTransaction = createAction<BraveWallet.TransactionInfo>('cancelTransaction')
export const speedupTransaction = createAction<BraveWallet.TransactionInfo>('speedupTransaction')
export const updateUnapprovedTransactionSpendAllowance = createAction<UpdateUnapprovedTransactionSpendAllowanceType>('updateUnapprovedTransactionSpendAllowance')
export const updateUnapprovedTransactionNonce = createAction<UpdateUnapprovedTransactionNonceType>('updateUnapprovedTransactionNonce')
export const setTransactionProviderError = createAction<SetTransactionProviderErrorType>('setTransactionProviderError')
export const queueNextTransaction = createAction('queueNextTransaction')

// gas
export const updateUnapprovedTransactionGasFields = createAction<UpdateUnapprovedTransactionGasFieldsType>('updateUnapprovedTransactionGasFields')
export const refreshGasEstimates = createAction<BraveWallet.TransactionInfo>('refreshGasEstimates')
export const setGasEstimates = createAction<BraveWallet.GasEstimation1559>('setGasEstimates')
export const setSolFeeEstimates = createAction<SolFeeEstimates>('setEstimatedSolFee')

// transaction history
export const transactionStatusChanged = createAction<TransactionStatusChanged>('transactionStatusChanged')
export const setAccountTransactions = createAction<AccountTransactions>('setAccountTransactions')

// active origin
export const activeOriginChanged = createAction<BraveWallet.OriginInfo>('activeOriginChanged')
export const removeSitePermission = createAction<RemoveSitePermissionPayloadType>('removeSitePermission')
export const addSitePermission = createAction<AddSitePermissionPayloadType>('addSitePermission')
export const setSitePermissions = createAction<SitePermissionsPayloadType>('setSitePermissions')

// batch updates
export const refreshBalancesAndPrices = createAction('refreshBalancesAndPrices')
export const balancesAndPricesRefreshed = createAction<BalancesAndPricesRefreshedPayload>('balancesAndPricesRefreshed')

export const refreshBalancesAndPriceHistory = createAction('refreshBalancesAndPriceHistory')
export const balancesAndPriceHistoryRefreshed = createAction<BalancesAndPricesHistoryRefreshedPayload>('balancesAndPriceHistoryRefreshed')

export const walletInfoUpdated = createAction<WalletInfoUpdatedPayload>('walletInfoUpdated')
export const walletDataInitialized = createAction<WalletDataInitializedPayload>('walletDataInitialized')
export const transactionHistoryRefreshed = createAction<TransactionHistoryRefreshedPayload>('transactionHistoryRefreshed')
