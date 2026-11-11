// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import {
  BraveWallet,
  UserAssetInfoType,
  WalletStatus,
} from '$wallet/constants/types'

// Constants
import { emptyRewardsInfo } from '$wallet/common/async/base-query-cache'

// Utils
import Amount from '$wallet/utils/amount'
import {
  computeFiatAmount,
  getPriceRequestsForTokens,
} from '$wallet/utils/pricing-utils'
import { getBalance } from '$wallet/utils/balance-utils'
import { getNetworkId } from '$wallet/common/slices/entities/network.entity'
import { networkSupportsAccount } from '$wallet/utils/network-utils'
import { getIsRewardsToken } from '$wallet/utils/rewards_utils'

// Hooks
import { useSafeWalletSelector } from './use-safe-selector'
import { useBalancesFetcher } from './use-balances-fetcher'
import { usePortfolioVisibleNetworks } from './use_portfolio_networks'
import { usePortfolioAccounts } from './use_portfolio_accounts'
import { usePersistedTokenSpotPricesQuery } from './use-persisted-spot-prices'

// Selectors
import { WalletSelectors } from '../selectors'

// Queries
import {
  useGetVisibleNetworksQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetRewardsInfoQuery,
  useGetUserTokensRegistryQuery,
} from '$wallet/common/slices/api.slice'
import {
  querySubscriptionOptions60s, //
} from '$wallet/common/slices/constants'
import {
  selectAllVisibleFungibleUserAssetsFromQueryResult, //
} from '$wallet/common/slices/entities/blockchain-token.entity'

export const usePortfolioAssets = (options?: { skipSpotPrices?: boolean }) => {
  const skipSpotPrices = options?.skipSpotPrices ?? false

  // Selectors
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)

  // Hooks
  const {
    filteredOutPortfolioNetworkKeys,
    visiblePortfolioNetworkIds,
    visiblePortfolioNetworks,
  } = usePortfolioVisibleNetworks()

  const { isLoadingAccounts, hasAccountsData, usersFilteredAccounts } =
    usePortfolioAccounts()

  // Queries
  const { data: networks } = useGetVisibleNetworksQuery()
  const { userVisibleTokensInfo, isLoadingUserTokens } =
    useGetUserTokensRegistryQuery(undefined, {
      selectFromResult: (result) => ({
        isLoadingUserTokens: result.isLoading,
        userVisibleTokensInfo:
          selectAllVisibleFungibleUserAssetsFromQueryResult(result),
      }),
    })
  const { data: defaultFiat } = useGetDefaultFiatCurrencyQuery()
  const {
    data: {
      balance: rewardsBalance,
      rewardsToken,
      status: rewardsStatus,
      rewardsAccount: externalRewardsAccount,
      rewardsNetwork: externalRewardsNetwork,
    } = emptyRewardsInfo,
    isLoading: isLoadingRewardsInfo,
  } = useGetRewardsInfoQuery()

  // Computed & Memos
  const isLoadingTokensOrRewards = isLoadingRewardsInfo || isLoadingUserTokens
  const isLoadingAccountsOrRewards = isLoadingRewardsInfo || isLoadingAccounts
  const displayRewardsInPortfolio = rewardsStatus === WalletStatus.kConnected

  const userTokensWithRewards = React.useMemo(() => {
    if (isLoadingTokensOrRewards) {
      // wait to render until we know which tokens to render
      return []
    }
    return displayRewardsInPortfolio && rewardsToken
      ? [rewardsToken].concat(userVisibleTokensInfo)
      : userVisibleTokensInfo
  }, [
    isLoadingTokensOrRewards,
    displayRewardsInPortfolio,
    rewardsToken,
    userVisibleTokensInfo,
  ])

  const displayRewardAccount =
    displayRewardsInPortfolio
    && externalRewardsNetwork
    && externalRewardsAccount
    && !filteredOutPortfolioNetworkKeys.includes(
      getNetworkId(externalRewardsNetwork),
    )

  const accountsListWithRewards = React.useMemo(() => {
    if (isLoadingAccountsOrRewards) {
      // wait to render until we know which accounts to render
      return []
    }
    return displayRewardAccount
      ? [externalRewardsAccount].concat(usersFilteredAccounts)
      : usersFilteredAccounts
  }, [
    isLoadingAccountsOrRewards,
    displayRewardAccount,
    externalRewardsAccount,
    usersFilteredAccounts,
  ])

  // Filters the user's tokens based on the users
  // filteredOutPortfolioNetworkKeys pref and visible networks.
  const visibleTokensForFilteredChains = React.useMemo(() => {
    return userTokensWithRewards.filter((token) =>
      visiblePortfolioNetworkIds.includes(getNetworkId(token)),
    )
  }, [userTokensWithRewards, visiblePortfolioNetworkIds])

  const skipBalances =
    isLoadingTokensOrRewards
    || usersFilteredAccounts.length === 0
    || visiblePortfolioNetworks.length === 0

  const { data: tokenBalancesRegistry, isLoading: isLoadingBalances } =
    // wait to see if we need rewards before fetching
    useBalancesFetcher(
      skipBalances
        ? skipToken
        : {
            accounts: usersFilteredAccounts,
            networks: visiblePortfolioNetworks,
          },
    )

  // This will scrape all the user's accounts and combine the asset balances
  // for a single asset
  const fullAssetBalance = React.useCallback(
    (asset: BraveWallet.BlockchainToken) => {
      if (!tokenBalancesRegistry) {
        return ''
      }

      const network = networks?.find(
        (network) =>
          network.coin === asset.coin && network.chainId === asset.chainId,
      )

      const amounts = usersFilteredAccounts
        .filter((account) => {
          return network && networkSupportsAccount(network, account.accountId)
        })
        .map((account) =>
          getBalance(account.accountId, asset, tokenBalancesRegistry),
        )

      // If a user has not yet created a FIL or SOL account,
      // we return 0 until they create an account
      if (amounts.length === 0) {
        return '0'
      }

      return amounts.reduce(function (a, b) {
        return a !== '' && b !== '' ? new Amount(a).plus(b).format() : ''
      })
    },
    [tokenBalancesRegistry, networks, usersFilteredAccounts],
  )

  // This looks at the users asset list and returns the full balance for
  // each asset
  const visibleAssetOptions: UserAssetInfoType[] = React.useMemo(() => {
    if (!tokenBalancesRegistry) {
      // wait for balances before computing this list
      return []
    }
    return visibleTokensForFilteredChains.map((asset) => {
      return {
        asset,
        assetBalance:
          getIsRewardsToken(asset) && rewardsBalance
            ? new Amount(rewardsBalance)
                .multiplyByDecimals(asset.decimals)
                .format()
            : fullAssetBalance(asset),
      }
    })
  }, [
    visibleTokensForFilteredChains,
    fullAssetBalance,
    rewardsBalance,
    tokenBalancesRegistry,
  ])

  const tokenPriceRequests = React.useMemo(
    () =>
      getPriceRequestsForTokens(
        visibleAssetOptions
          .filter(({ assetBalance }) => new Amount(assetBalance).gt(0))
          .map(({ asset }) => asset),
      ),
    [visibleAssetOptions],
  )

  const { data: spotPrices = [], isLoading: isLoadingSpotPrices } =
    usePersistedTokenSpotPricesQuery(
      !skipSpotPrices && tokenPriceRequests.length && defaultFiat
        ? { requests: tokenPriceRequests, vsCurrency: defaultFiat }
        : skipToken,
      querySubscriptionOptions60s,
    )

  // This will scrape all of the user's accounts and combine the fiat value
  // for every asset
  const fullPortfolioFiatBalance = React.useMemo((): Amount => {
    if (
      !hasInitialized
      || !isWalletCreated
      || isLoadingTokensOrRewards
      || isLoadingAccounts
      || !hasAccountsData
      || networks === undefined
    ) {
      return Amount.empty()
    }

    // Balances will never arrive: nothing to fetch, or the query is skipped
    // because the wallet is locked and there is no persisted registry.
    if (
      usersFilteredAccounts.length === 0
      || visiblePortfolioNetworks.length === 0
      || (isWalletLocked && !tokenBalancesRegistry)
    ) {
      return Amount.zero()
    }

    if (!tokenBalancesRegistry || isLoadingBalances || isLoadingSpotPrices) {
      return Amount.empty()
    }

    if (
      visibleAssetOptions.length === 0
      || accountsListWithRewards.length === 0
    ) {
      return Amount.zero()
    }

    const visibleAssetFiatBalances = visibleAssetOptions.map((item) => {
      return computeFiatAmount({
        spotPrices,
        value: item.assetBalance,
        token: item.asset,
      })
    })

    const grandTotal = visibleAssetFiatBalances.reduce(function (a, b) {
      return a.plus(b)
    })
    return grandTotal
  }, [
    hasInitialized,
    isWalletCreated,
    isLoadingTokensOrRewards,
    isLoadingAccounts,
    hasAccountsData,
    networks,
    usersFilteredAccounts,
    visiblePortfolioNetworks,
    isWalletLocked,
    tokenBalancesRegistry,
    isLoadingBalances,
    isLoadingSpotPrices,
    visibleAssetOptions,
    accountsListWithRewards,
    spotPrices,
  ])

  const formattedFullPortfolioFiatBalance = React.useMemo(() => {
    return !fullPortfolioFiatBalance.isUndefined() && defaultFiat
      ? fullPortfolioFiatBalance.compactAsFiat(defaultFiat)
      : ''
  }, [fullPortfolioFiatBalance, defaultFiat])

  return {
    formattedFullPortfolioFiatBalance,
    fullPortfolioFiatBalance,
    defaultFiat,
    visibleAssetOptions,
    spotPrices,
    tokenBalancesRegistry,
    visiblePortfolioNetworks,
    visibleTokensForFilteredChains,
    accountsListWithRewards,
    usersFilteredAccounts,
  }
}
