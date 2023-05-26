// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createApi } from '@reduxjs/toolkit/dist/query'
import { createTransform } from 'redux-persist'
import {
  InvalidationState,
  QueryState,
  SubscriptionState
} from '@reduxjs/toolkit/dist/query/core/apiState'

// types
import type {
  WalletState,
  PanelState,
  PageState,
  UIState
} from '../../constants/types'
import type {
  WalletApiQueryEndpointName,
  WalletApiSliceState
} from '../slices/api.slice'
import { ApiTagTypeName } from '../slices/api-base.slice'

type Subset<T, U extends T> = U;

type Whitelist<STATE, BANNED_KEYS = ''> = Array<
  Exclude<keyof STATE, BANNED_KEYS>
>

type ApiSliceRootState = ReturnType<ReturnType<typeof createApi>['reducer']>

/**
 * Persisting the Api Slice is not a recommended practice
 * Attempts were made to persist a subset of this data, but bugs were found
 *
 * The most important issue is that subscriptions should not all be persisted
 */
type BlacklistedApiSliceRootStateKeys = Subset<
  keyof ApiSliceRootState,
  | 'config' // cache configuration
  | 'mutations' // fulfilled mutations should not be persisted
  | 'provided' // query tags for invalidation
  | 'queries' // fulfilled (whitelisted) queries
  // do not persist subscriptions
  // since subscribing components may no longer exist
  | 'subscriptions'
>

export const apiStatePersistorWhitelist: Array<
  Exclude<keyof ApiSliceRootState, BlacklistedApiSliceRootStateKeys>
> = [] // purposely empty for now

type BlacklistedWalletApiQueryEndpointName = Subset<
  WalletApiQueryEndpointName,
  | 'getTransactions' // prevent tampering with Pending Txs
  | 'getDefaultFiatCurrency' // defaults may have changed since last launch
  | 'getGasEstimation1559' // network fees constantly change
  | 'getSelectedAccountAddress' // selections may have changed
  | 'getSelectedChain' // selections may have changed
  | 'getSolanaEstimatedFee' // network fees constantly change
  | 'getAccountTokenCurrentBalance' // prefer showing latest balances
  | 'getCombinedTokenBalanceForAllAccounts' // prefer showing latest balances
  | 'getTokenBalancesForChainId' // prefer showing latest balances
>

type WhitelistedWalletApiQueryEndpointName = Exclude<
  WalletApiQueryEndpointName,
  BlacklistedWalletApiQueryEndpointName
>

export const apiEndpointWhitelist: WhitelistedWalletApiQueryEndpointName[] = [
  'getAccountInfosRegistry', // persist wallet addresses
  'getAddressByteCode', // persist contract bytecode lookups
  'getERC721Metadata', // persist nft metadata
  'getNetworksRegistry', // persist list of networks
  'getSwapSupportedNetworkIds', // persist which networks support Brave Swap
  'getTokenSpotPrices', // persist token spot price
  'getTokensRegistry', // persist known tokens registry
  'getUserTokensRegistry' // persist user tokens registry
]

type BlacklistedProvidedTagName = Subset<
  ApiTagTypeName,
  | 'AccountTokenCurrentBalance' // prefer latest balance
  | 'BraveRewards-Enabled' // may have changed
  | 'BraveRewards-ExternalWallet' // may have changed
  | 'BraveRewards-RewardsBalance' // may have changed
  | 'CombinedTokenBalanceForAllAccounts' // prefer latest balance
  | 'DefaultFiatCurrency' // may have changed
  | 'GasEstimation1559' // use latest gas
  | 'Network' // networks may have changed from settings
  | 'NftDiscoveryEnabledStatus' // may have changed from settings
  | 'SolanaEstimatedFees' // use latest gas
  | 'TokenBalancesForChainId' // prefer latest balance
  | 'TransactionSimulationsEnabled' // may change from settings
  | 'Transactions' // prevent tampering with pending transactions
  | 'WalletInfo' // settings may have changed
  | 'UNKNOWN_ERROR' // don't persist errors
  | 'UNAUTHORIZED' // don't persist errors
>

type WhitelistedProvidedTagName = Exclude<
  ApiTagTypeName,
  BlacklistedProvidedTagName
>

export const apiCacheTagsWhitelist: WhitelistedProvidedTagName[] = [
  'AccountInfos',
  'ERC721Metadata', // save NFT metadata
  'KnownBlockchainTokens', // save tokens registry
  'TokenSpotPrices', // save spot price,
  'UserBlockchainTokens', // save user tokens
]

type BlacklistedPageStateKey = Subset<
  keyof PageState,
  | 'enablingAutoPin' // unused state
  | 'hasInitialized' // unused state
  | 'importAccountError' // don't persist import errors
  | 'importWalletAttempts' // do not rely on persisted storage for attempts
  | 'importWalletError' // don't persist import errors
  | 'invalidMnemonic' // don't persist errors
  | 'isAutoPinEnabled' // selection may have changed
  | 'isCryptoWalletsInitialized' // selection may have changed
  | 'isImportWalletsCheckComplete' // importable wallets may have changed
  | 'isLocalIpfsNodeRunning' // selection may have changed
  | 'isMetaMaskInitialized' // selection may have changed
  | 'mnemonic' // do not store private data
  | 'nftMetadataError' // do not persist errors
  | 'nftsPinningStatus' // pinning may have been disabled since last launch
  | 'pinStatusOverview' // unused state
  | 'setupStillInProgress' // start onboarding again if not completed
  | 'showRecoveryPhrase' // do not show private data on app relaunch
>

export const pageStatePersistorWhitelist: Whitelist<
  PageState,
  BlacklistedPageStateKey
> = [
  'isFetchingNFTMetadata', // save NFT metadata
  'isFetchingPriceHistory', // save price history
  'nftMetadata', // save NFT metadata
  'portfolioPriceHistory', // save portfolio historical price data
  'selectedAsset', // save asset selection
  'selectedAssetPriceHistory', // save price history of selected asset
  'selectedCoinMarket', // save selection
  'selectedTimeline', // save selection
  'showAddModal', // allow resuming adding an asset
  'showIsRestoring', // allow resuming wallet restoration
  'walletTermsAcknowledged' // persist terms acknowledgment
]

type BlacklistedPanelStateKey = Subset<
  keyof PanelState,
  | 'addChainRequest' // prevent tampering with pending requests
  | 'connectingAccounts' // prevent tampering with pending connections
  | 'connectToSiteOrigin' // prevent tampering with pending connections
  | 'decryptRequest' // prevent tampering with pending requests
  | 'getEncryptionPublicKeyRequest' // prevent tampering with pending requests
  | 'hardwareWalletCode' // prevent tampering with pending connections
  | 'hasInitialized' // unused state
  | 'lastSelectedPanel' // ux choice, always launch to home panel
  | 'panelTitle'  // ux choice, always launch to home panel
  | 'selectedPanel'  // ux choice, always launch to home panel
  | 'signAllTransactionsRequests' // prevent tampering with pending transactions
  | 'signMessageData' // prevent tampering with pending signature requests
  | 'signTransactionRequests' // prevent tampering with pending sign requests
  | 'suggestedTokenRequest' // prevent tampering token additions
  | 'switchChainRequest' // prevent tampering with switch chain requests
>

// intentionally empty for now
export const panelStatePersistorWhitelist: Whitelist<
  PanelState,
  BlacklistedPanelStateKey
> = []

type BlacklistedWalletStateKey = Subset<
  keyof WalletState,
  | 'accounts' // soon to be deprecated by api slice queries
  | 'activeOrigin' // prevent tampering with active origin
  | 'addUserAssetError' // don't persist errors
  | 'assetAutoDiscoveryCompleted' // always check for new assets
  | 'coinMarketData' // always show latest market data
  | 'connectedAccounts' // prevent tampering with connections
  | 'defaultCurrencies' // may have changed from Web3 settings
  | 'defaultEthereumWallet' // may have changed since app relaunch
  | 'defaultSolanaWallet' // may have changed since app relaunch
  | 'favoriteApps' // prevent tampering with favorites
  | 'gasEstimates' // gas prices change constantly
  | 'hasFeeEstimatesError' // skip persisting errors
  | 'hasIncorrectPassword' // skip persisting errors
  | 'hasInitialized' // skip persisting initialization state
  | 'isFilecoinEnabled' // may have changed from flags
  | 'isLoadingCoinMarketData' // always show latest market data
  | 'isMetaMaskInstalled' // may have changed since app relaunch
  | 'isNftPinningFeatureEnabled' // may have changed from Web3 settings
  | 'isPanelV2FeatureEnabled' // may have changed from flags
  | 'isSolanaEnabled' // may have changed from flags
  | 'isSolanaEnabled' // may have changed from flags
  | 'isWalletLocked' // prevent unlock without password
  | 'passwordAttempts' // prevent tampering with attempts count
  | 'solFeeEstimates' // gas prices change constantly
>



type BlacklistedUiStateKey = Subset<
  keyof UIState,
  | 'selectedPendingTransactionId' // don't persist panel selections
  | 'transactionProviderErrorRegistry' // don't persist errors
>

// intentionally empty for now
export const uiStatePersistorWhitelist: Whitelist<
  UIState,
  BlacklistedUiStateKey
> = []

export const walletStatePersistorWhitelist: Whitelist<
  WalletState,
  BlacklistedWalletStateKey
> = [
  'fullTokenList', // save known tokens list
  'isFetchingPortfolioPriceHistory', // save loading status of price history
  'isWalletBackedUp', // save acknowledgement of wallet backup
  'isWalletCreated', // save that the wallet was created
  'onRampCurrencies', // save onramp currency list
  'portfolioPriceHistory', // save portfolio price history
  'selectedAccountFilter', // save selection
  'selectedAssetFilter', // save selection
  'selectedCurrency', // save selection
  'selectedNetworkFilter', // save selection
  'selectedPortfolioTimeline', // save selection
  'userVisibleTokensInfo' // save user tokens list
]

/**
 * Used to transform the state before persisting the value to storage
 */
export const privacyAndSecurityTransform = createTransform<
  WalletApiSliceState[keyof WalletApiSliceState],
  WalletApiSliceState[keyof WalletApiSliceState],
  WalletApiSliceState,
  WalletApiSliceState
>(
  // transform state before it is serialized and persisted
  (stateToSerialize, key) => {
    if (key === 'queries') {
      return getWhitelistedQueryData(
        stateToSerialize as WalletApiSliceState['queries']
      )
    }
    if (key === 'config') {
      return stateToSerialize
    }
    if (key === 'provided') {
      return getWhitelistedProvidedTagsData(
        stateToSerialize as WalletApiSliceState['provided']
      )
    }
    if (key === 'mutations') {
      return {}
    }
    if (key === 'subscriptions') {
      return getWhitelistedQueryData(
        stateToSerialize as WalletApiSliceState['subscriptions'],
        'subscriptions'
      )
    }
    return stateToSerialize
  },
  // transform before rehydration
  (stateToRehydrate, key) => {
    if (key === 'queries') {
      return getWhitelistedQueryData(
        stateToRehydrate as WalletApiSliceState['queries']
      )
    }
    if (key === 'config') {
      return stateToRehydrate
    }
    if (key === 'provided') {
      return getWhitelistedProvidedTagsData(
        stateToRehydrate as WalletApiSliceState['provided']
      )
    }
    if (key === 'mutations') {
      return {}
    }
    if (key === 'subscriptions') {
      return getWhitelistedQueryData(
        stateToRehydrate as WalletApiSliceState['subscriptions'],
        'subscriptions'
      )
    }
    return stateToRehydrate
  },
  {
    whitelist: apiStatePersistorWhitelist
  }
)

export function getWhitelistedQueryData(
  stateToSerialize: QueryState<any> | SubscriptionState,
  mode = 'query'
) {
  const entries = Object.entries(stateToSerialize)

  const whitelistedEntries = entries.filter(([queryKey, queryValue]) => {
    return apiEndpointWhitelist.some((endpoint) =>
      (queryKey as WhitelistedWalletApiQueryEndpointName).includes(endpoint)
    )
  })

  const whitelistedState = whitelistedEntries.reduce(
    (acc, [queryKey, queryValue]) => {
      if (queryKey !== undefined && queryValue !== null) {
        acc[queryKey] = queryValue
      }
      return acc
    },
    {}
  )

  return whitelistedState
}

export function getWhitelistedProvidedTagsData(
  stateToSerialize: InvalidationState<ApiTagTypeName>
) {
  const entries = Object.entries(stateToSerialize)

  const whitelistedEntries = entries.filter(([queryKey, queryValue]) => {
    return apiCacheTagsWhitelist.some((tag) =>
      (queryKey as WhitelistedProvidedTagName).includes(tag)
    )
  })

  const whitelistedState = whitelistedEntries.reduce(
    (acc, [queryKey, queryValue]) => {
      if (queryKey !== undefined && queryValue !== null) {
        acc[queryKey] = queryValue
      }
      return acc
    },
    {}
  )

  return whitelistedState
}
