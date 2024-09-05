// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useHistory, useLocation } from 'react-router'
import { Route, Switch, Redirect } from 'react-router-dom'

// selectors
import { useSafeUISelector } from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'

// hooks
import {
  useBalancesFetcher //
} from '../../../../common/hooks/use-balances-fetcher'
import {
  useLocalStorage,
  useSyncedLocalStorage
} from '../../../../common/hooks/use_local_storage'
import {
  usePortfolioVisibleNetworks //
} from '../../../../common/hooks/use_portfolio_networks'
import {
  usePortfolioAccounts //
} from '../../../../common/hooks/use_portfolio_accounts'

// Constants
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../common/constants/local-storage-keys'
import {
  BraveWallet,
  UserAssetInfoType,
  WalletRoutes,
  WalletStatus
} from '../../../../constants/types'
import { emptyRewardsInfo } from '../../../../common/async/base-query-cache'

// Utils
import Amount from '../../../../utils/amount'
import {
  computeFiatAmount,
  getTokenPriceAmountFromRegistry,
  getPriceIdForToken
} from '../../../../utils/pricing-utils'
import { getBalance } from '../../../../utils/balance-utils'
import { getAssetIdKey } from '../../../../utils/asset-utils'
import {
  networkEntityAdapter //
} from '../../../../common/slices/entities/network.entity'
import { networkSupportsAccount } from '../../../../utils/network-utils'
import { getIsRewardsToken } from '../../../../utils/rewards_utils'
import {
  getStoredPortfolioTimeframe //
} from '../../../../utils/local-storage-utils'
import { makePortfolioAssetRoute } from '../../../../utils/routes-utils'

// Options
import { PortfolioNavOptions } from '../../../../options/nav-options'
import {
  AccountsGroupByOption, //
  NoneGroupByOption
} from '../../../../options/group-assets-by-options'

// Components
import { LoadingSkeleton } from '../../../shared/loading-skeleton/index'
import {
  SegmentedControl //
} from '../../../shared/segmented_control/segmented_control'
import { PortfolioAssetItem } from '../../portfolio-asset-item/index'
import { TokenLists } from './components/token-lists/token-list'
import {
  PortfolioOverviewChart //
} from './components/portfolio-overview-chart/portfolio-overview-chart'
import ColumnReveal from '../../../shared/animated-reveals/column-reveal'
import { Nfts } from '../nfts/components/nfts'
import {
  BuySendSwapDepositNav //
} from './components/buy-send-swap-deposit-nav/buy-send-swap-deposit-nav'
import {
  PortfolioFiltersModal //
} from '../../popup-modals/filter-modals/portfolio-filters-modal'

// Styled Components
import {
  BalanceText,
  PercentBubble,
  FiatChange,
  ControlsRow,
  BalanceAndButtonsWrapper,
  BalanceAndChangeWrapper,
  BackgroundWatermark,
  BalanceAndLineChartWrapper
} from './style'
import { Column, Row, HorizontalSpace } from '../../../shared/style'

// Queries
import {
  useGetVisibleNetworksQuery,
  useGetPricesHistoryQuery,
  useGetTokenSpotPricesQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetRewardsInfoQuery,
  useGetUserTokensRegistryQuery
} from '../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s //
} from '../../../../common/slices/constants'
import {
  selectAllVisibleFungibleUserAssetsFromQueryResult //
} from '../../../../common/slices/entities/blockchain-token.entity'

export const PortfolioOverview = () => {
  // routing
  const history = useHistory()
  const location = useLocation()
  const isCollectionView = location.pathname.includes(
    WalletRoutes.PortfolioNFTCollection.replace(':collectionName', '')
  )

  // custom hooks
  const {
    filteredOutPortfolioNetworkKeys,
    visiblePortfolioNetworkIds,
    visiblePortfolioNetworks
  } = usePortfolioVisibleNetworks()

  const { isLoadingAccounts, usersFilteredAccounts } = usePortfolioAccounts()

  // local-storage
  const [selectedGroupAssetsByItem] = useLocalStorage<string>(
    LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_ASSETS_BY,
    NoneGroupByOption.id
  )
  const [hidePortfolioSmallBalances] = useLocalStorage<boolean>(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_SMALL_BALANCES,
    false
  )
  const [hidePortfolioBalances] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
    false
  )
  const [hidePortfolioNFTsTab] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB,
    false
  )
  const [hidePortfolioGraph] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_GRAPH_HIDDEN,
    false
  )

  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // queries
  const { data: networks } = useGetVisibleNetworksQuery()
  const { userVisibleTokensInfo, isLoadingUserTokens } =
    useGetUserTokensRegistryQuery(undefined, {
      selectFromResult: (result) => ({
        isLoadingUserTokens: result.isLoading,
        userVisibleTokensInfo:
          selectAllVisibleFungibleUserAssetsFromQueryResult(result)
      })
    })
  const { data: defaultFiat } = useGetDefaultFiatCurrencyQuery()
  const {
    data: {
      balance: rewardsBalance,
      rewardsToken,
      status: rewardsStatus,
      rewardsAccount: externalRewardsAccount,
      rewardsNetwork: externalRewardsNetwork
    } = emptyRewardsInfo,
    isLoading: isLoadingRewardsInfo
  } = useGetRewardsInfoQuery()

  const isLoadingTokensOrRewards = isLoadingRewardsInfo || isLoadingUserTokens
  const isLoadingAccountsOrRewards = isLoadingRewardsInfo || isLoadingAccounts

  // State
  const [showPortfolioSettings, setShowPortfolioSettings] =
    React.useState<boolean>(false)
  const [selectedTimeframe, setSelectedTimeframe] =
    React.useState<BraveWallet.AssetPriceTimeframe>(getStoredPortfolioTimeframe)

  // Computed & Memos
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
    userVisibleTokensInfo
  ])

  const displayRewardAccount =
    displayRewardsInPortfolio &&
    externalRewardsNetwork &&
    externalRewardsAccount &&
    !filteredOutPortfolioNetworkKeys.includes(
      networkEntityAdapter.selectId(externalRewardsNetwork).toString()
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
    usersFilteredAccounts
  ])

  // Filters the user's tokens based on the users
  // filteredOutPortfolioNetworkKeys pref and visible networks.
  const visibleTokensForFilteredChains = React.useMemo(() => {
    return userTokensWithRewards.filter((token) =>
      visiblePortfolioNetworkIds.includes(
        networkEntityAdapter.selectId(token).toString()
      )
    )
  }, [userTokensWithRewards, visiblePortfolioNetworkIds])

  const { data: tokenBalancesRegistry } =
    // wait to see if we need rewards before fetching
    useBalancesFetcher(
      isLoadingTokensOrRewards ||
        usersFilteredAccounts.length === 0 ||
        visiblePortfolioNetworks.length === 0
        ? skipToken
        : {
            accounts: usersFilteredAccounts,
            networks: visiblePortfolioNetworks
          }
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
          network.coin === asset.coin && network.chainId === asset.chainId
      )

      const amounts = usersFilteredAccounts
        .filter((account) => {
          return network && networkSupportsAccount(network, account.accountId)
        })
        .map((account) =>
          getBalance(account.accountId, asset, tokenBalancesRegistry)
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
    [tokenBalancesRegistry, networks, usersFilteredAccounts]
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
            : fullAssetBalance(asset)
      }
    })
  }, [
    visibleTokensForFilteredChains,
    fullAssetBalance,
    rewardsBalance,
    tokenBalancesRegistry
  ])

  const tokenPriceIds = React.useMemo(
    () =>
      visibleAssetOptions
        .filter(({ assetBalance }) => new Amount(assetBalance).gt(0))
        .map(({ asset }) => getPriceIdForToken(asset)),
    [visibleAssetOptions]
  )

  const { data: spotPriceRegistry, isLoading: isLoadingSpotPrices } =
    useGetTokenSpotPricesQuery(
      !isCollectionView && tokenPriceIds.length && defaultFiat
        ? { ids: tokenPriceIds, toCurrency: defaultFiat }
        : skipToken,
      querySubscriptionOptions60s
    )

  const {
    data: portfolioPriceHistory,
    isFetching: isFetchingPortfolioPriceHistory
  } = useGetPricesHistoryQuery(
    !isCollectionView &&
      visibleTokensForFilteredChains.length &&
      tokenBalancesRegistry &&
      defaultFiat
      ? {
          tokens: visibleTokensForFilteredChains,
          timeframe: selectedTimeframe,
          vsAsset: defaultFiat,
          tokenBalancesRegistry
        }
      : skipToken
  )

  // This will scrape all of the user's accounts and combine the fiat value
  // for every asset
  const fullPortfolioFiatBalance = React.useMemo((): Amount => {
    if (
      !tokenBalancesRegistry ||
      isLoadingSpotPrices ||
      isLoadingTokensOrRewards
    ) {
      return Amount.empty()
    }

    if (
      visibleAssetOptions.length === 0 ||
      visiblePortfolioNetworks.length === 0 ||
      accountsListWithRewards.length === 0
    ) {
      return Amount.zero()
    }

    const visibleAssetFiatBalances = visibleAssetOptions.map((item) => {
      return computeFiatAmount({
        spotPriceRegistry,
        value: item.assetBalance,
        token: item.asset
      })
    })

    const grandTotal = visibleAssetFiatBalances.reduce(function (a, b) {
      return a.plus(b)
    })
    return grandTotal
  }, [
    tokenBalancesRegistry,
    visiblePortfolioNetworks,
    visibleAssetOptions,
    spotPriceRegistry,
    accountsListWithRewards,
    isLoadingTokensOrRewards,
    isLoadingSpotPrices
  ])

  const formattedFullPortfolioFiatBalance = React.useMemo(() => {
    return !fullPortfolioFiatBalance.isUndefined() && defaultFiat
      ? fullPortfolioFiatBalance.formatAsFiat(defaultFiat)
      : ''
  }, [fullPortfolioFiatBalance, defaultFiat])

  const change = React.useMemo(() => {
    if (
      portfolioPriceHistory &&
      portfolioPriceHistory.length !== 0 &&
      !fullPortfolioFiatBalance.isUndefined()
    ) {
      const oldestValue = new Amount(portfolioPriceHistory[0].close)
      return {
        difference: fullPortfolioFiatBalance.isZero()
          ? Amount.zero()
          : fullPortfolioFiatBalance.minus(oldestValue),
        oldestValue
      }
    }

    // Case when portfolio change should not be displayed
    return {
      difference: Amount.zero(),
      oldestValue: Amount.empty()
    }
  }, [portfolioPriceHistory, fullPortfolioFiatBalance])

  const percentageChange = React.useMemo(() => {
    const { difference, oldestValue } = change
    if (oldestValue.isUndefined()) {
      return ''
    }

    if (
      !isFetchingPortfolioPriceHistory &&
      oldestValue.isZero() &&
      difference.isZero()
    ) {
      return '0'
    }

    return `${difference.div(oldestValue).times(100).format(2)}`
  }, [change, isFetchingPortfolioPriceHistory])

  const fiatValueChange = React.useMemo(() => {
    const { difference, oldestValue } = change
    if (oldestValue.isUndefined()) {
      return ''
    }

    return difference.formatAsFiat(defaultFiat, 2)
  }, [defaultFiat, change])

  const isPortfolioDown = new Amount(percentageChange).lt(0)

  // methods
  const onSelectAsset = React.useCallback(
    (asset: BraveWallet.BlockchainToken) => {
      history.push(makePortfolioAssetRoute(false, getAssetIdKey(asset)))
    },
    [history]
  )

  const tokenLists = React.useMemo(() => {
    return (
      <TokenLists
        userAssetList={visibleAssetOptions}
        estimatedItemSize={58}
        horizontalPadding={20}
        onShowPortfolioSettings={() => setShowPortfolioSettings(true)}
        hideSmallBalances={hidePortfolioSmallBalances}
        networks={visiblePortfolioNetworks}
        accounts={accountsListWithRewards}
        tokenBalancesRegistry={tokenBalancesRegistry}
        spotPriceRegistry={spotPriceRegistry}
        renderToken={({ item, account }) => (
          <PortfolioAssetItem
            action={() => onSelectAsset(item.asset)}
            key={getAssetIdKey(item.asset)}
            assetBalance={
              !tokenBalancesRegistry
                ? ''
                : selectedGroupAssetsByItem === AccountsGroupByOption.id &&
                  !getIsRewardsToken(item.asset)
                ? getBalance(
                    account?.accountId,
                    item.asset,
                    tokenBalancesRegistry
                  )
                : item.assetBalance
            }
            account={
              selectedGroupAssetsByItem === AccountsGroupByOption.id
                ? account
                : undefined
            }
            token={item.asset}
            hideBalances={hidePortfolioBalances}
            spotPrice={
              spotPriceRegistry
                ? getTokenPriceAmountFromRegistry(
                    spotPriceRegistry,
                    item.asset
                  ).format()
                : tokenBalancesRegistry
                ? '0'
                : ''
            }
            isGrouped={selectedGroupAssetsByItem !== NoneGroupByOption.id}
          />
        )}
      />
    )
  }, [
    visibleAssetOptions,
    hidePortfolioSmallBalances,
    visiblePortfolioNetworks,
    accountsListWithRewards,
    onSelectAsset,
    selectedGroupAssetsByItem,
    hidePortfolioBalances,
    spotPriceRegistry,
    tokenBalancesRegistry
  ])

  // render
  return (
    <>
      {!isCollectionView && (
        <>
          <BalanceAndLineChartWrapper
            fullWidth={true}
            justifyContent='flex-start'
          >
            {isPanel && <BackgroundWatermark />}
            <BalanceAndButtonsWrapper
              fullWidth={true}
              alignItems='center'
              padding='40px 32px'
            >
              <BalanceAndChangeWrapper>
                {formattedFullPortfolioFiatBalance !== '' ? (
                  <BalanceText>
                    {hidePortfolioBalances
                      ? '******'
                      : formattedFullPortfolioFiatBalance}
                  </BalanceText>
                ) : (
                  <Column padding='9px 0px'>
                    <LoadingSkeleton
                      width={150}
                      height={36}
                    />
                  </Column>
                )}
                <Row
                  alignItems='center'
                  justifyContent='center'
                  width='unset'
                >
                  {fiatValueChange !== '' ? (
                    <>
                      <FiatChange isDown={isPortfolioDown}>
                        {hidePortfolioBalances
                          ? '*****'
                          : `${isPortfolioDown ? '' : '+'}${fiatValueChange}`}
                      </FiatChange>
                      <PercentBubble isDown={isPortfolioDown}>
                        {hidePortfolioBalances
                          ? '*****'
                          : `${isPortfolioDown ? '' : '+'}${percentageChange}%`}
                      </PercentBubble>
                    </>
                  ) : (
                    <>
                      <LoadingSkeleton
                        width={55}
                        height={24}
                      />
                      <HorizontalSpace space='8px' />
                      <LoadingSkeleton
                        width={55}
                        height={24}
                      />
                    </>
                  )}
                </Row>
              </BalanceAndChangeWrapper>
              <BuySendSwapDepositNav />
            </BalanceAndButtonsWrapper>
            <ColumnReveal hideContent={hidePortfolioGraph}>
              <PortfolioOverviewChart
                timeframe={selectedTimeframe}
                onTimeframeChanged={setSelectedTimeframe}
                hasZeroBalance={fullPortfolioFiatBalance.isZero()}
                portfolioPriceHistory={portfolioPriceHistory}
                isLoading={
                  isFetchingPortfolioPriceHistory || !portfolioPriceHistory
                }
              />
            </ColumnReveal>
          </BalanceAndLineChartWrapper>
          <ControlsRow controlsHidden={hidePortfolioNFTsTab}>
            {!hidePortfolioNFTsTab && (
              <SegmentedControl
                navOptions={PortfolioNavOptions}
                maxWidth='384px'
              />
            )}
          </ControlsRow>
        </>
      )}

      <Switch>
        <Route
          path={WalletRoutes.PortfolioAssets}
          exact
        >
          {tokenLists}
        </Route>

        <Route
          path={WalletRoutes.AddAssetModal}
          exact
        >
          {tokenLists}
        </Route>

        <Route
          path={WalletRoutes.PortfolioNFTs}
          exact
        >
          <Nfts
            networks={visiblePortfolioNetworks}
            accounts={usersFilteredAccounts}
            onShowPortfolioSettings={() => setShowPortfolioSettings(true)}
          />
        </Route>

        <Route
          path={WalletRoutes.Portfolio}
          exact={true}
          render={() => <Redirect to={WalletRoutes.PortfolioAssets} />}
        />
      </Switch>

      {showPortfolioSettings && (
        <PortfolioFiltersModal
          onSave={() => {
            // reset to first page after filters change
            const newParams = new URLSearchParams(location.search)
            newParams.delete('page')
            history.push({
              ...location,
              search: `?${newParams.toString()}`
            })
          }}
          onClose={() => {
            setShowPortfolioSettings(false)
          }}
        />
      )}
    </>
  )
}

export default PortfolioOverview
