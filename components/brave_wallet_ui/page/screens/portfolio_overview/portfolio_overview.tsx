// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useHistory, useLocation } from 'react-router'
import { Route, Switch } from 'react-router-dom'

// Selectors
import {
  useSafeUISelector, //
} from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// hooks
import {
  useLocalStorage,
  useSyncedLocalStorage,
} from '../../../common/hooks/use_local_storage'
import { usePortfolioAssets } from '$wallet/common/hooks/use_portfolio_assets'

// Constants
import {
  LOCAL_STORAGE_KEYS, //
} from '../../../common/constants/local-storage-keys'
import { BraveWallet, WalletRoutes } from '../../../constants/types'

// Utils
import Amount from '../../../utils/amount'
import {
  computeFiatAmount,
  getTokenPriceAmountFromRegistry,
} from '../../../utils/pricing-utils'
import { getBalance } from '../../../utils/balance-utils'
import { getAssetIdKey } from '../../../utils/asset-utils'
import { getIsRewardsToken } from '../../../utils/rewards_utils'
import {
  getStoredPortfolioTimeframe, //
} from '../../../utils/local-storage-utils'
import { makePortfolioAssetRoute } from '../../../utils/routes-utils'

// Options
import {
  PortfolioNavOptions,
  PortfolioNavOptionsNoNFTsTab,
} from '../../../options/nav-options'
import {
  AccountsGroupByOption, //
  NoneGroupByOption,
} from '../../../options/group-assets-by-options'

// Components
import { LoadingSkeleton } from '../../../components/shared/loading-skeleton/index'
import {
  SegmentedControl, //
} from '../../../components/shared/segmented_control/segmented_control'
import { PortfolioAssetItem } from '$wallet/page/components/portfolio_asset_item/portfolio_asset_item'
import { TokenLists } from './components/token_lists/token_list'
import {
  PortfolioOverviewChart, //
} from './components/portfolio_overview_chart/portfolio_overview_chart'
import ColumnReveal from '../../../components/shared/animated-reveals/column-reveal'
import { Nfts } from '../nfts/nfts'
import {
  BuySendSwapDepositNav, //
} from './components/buy_send_swap_deposit_nav/buy_send_swap_deposit_nav'
import {
  PortfolioFiltersModal, //
} from '../../../components/desktop/popup-modals/filter-modals/portfolio-filters-modal'
import {
  TransactionsScreen, //
} from '../transactions/transactions-screen'
import {
  WalletPageWrapper, //
} from '$wallet/page/components/wallet_page_wrapper/wallet_page_wrapper'
import {
  PortfolioOverviewHeader, //
} from '$wallet/page/components/card_headers/portfolio_overview_header'
import { Banners } from '$wallet/page/components/banners/banners'
import {
  LastPricesUpdatedTooltip, //
} from '../../../components/shared/last_prices_updated_tooltip/last_prices_updated_tooltip'
import { GettingStarted } from './components/getting_started/getting_started'
import { PortfolioValueChange } from './components/portfolio_value_change/portfolio_value_change'

// Styled Components
import {
  ControlsRow,
  BalanceAndButtonsWrapper,
  BalanceAndChangeWrapper,
  BalanceAndLineChartWrapper,
  ActivityWrapper,
} from './portfolio_overview.style'
import {
  Text,
  Column,
  DefaultPageWrapper,
} from '../../../components/shared/style'

// Queries
import { useGetPricesHistoryQuery } from '../../../common/slices/api.slice'
import {
  PortfolioOverviewDistribution,
  PortfolioOverviewDistributionData,
} from './components/portfolio_overview_distribution/portfolio_overview_distribution'

const DISTRIBUTION_LIMIT = 3

export const PortfolioOverview = () => {
  // routing
  const history = useHistory()
  const location = useLocation()
  const isCollectionView = location.pathname.includes(
    WalletRoutes.PortfolioNFTCollection.replace(':collectionName', ''),
  )

  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isMobile = useSafeUISelector(UISelectors.isMobile)
  const isSidePanel = useSafeUISelector(UISelectors.isSidePanel)
  const isMobileOrPanel = isMobile || isPanel

  // custom hooks
  const {
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
  } = usePortfolioAssets({
    skipSpotPrices: isCollectionView,
  })

  // local-storage
  const [selectedGroupAssetsByItem] = useLocalStorage<string>(
    LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_ASSETS_BY,
    NoneGroupByOption.id,
  )
  const [hidePortfolioSmallBalances] = useLocalStorage<boolean>(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_SMALL_BALANCES,
    false,
  )
  const [hidePortfolioBalances] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
    false,
  )
  const [hidePortfolioNFTsTab] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB,
    false,
  )
  const [hidePortfolioGraph] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_GRAPH_HIDDEN,
    true,
  )

  const [hidePortfolioDistribution] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_DISTRIBUTION_HIDDEN,
    true,
  )

  // State
  const [showPortfolioSettings, setShowPortfolioSettings] =
    React.useState<boolean>(false)
  const [selectedTimeframe, setSelectedTimeframe] =
    React.useState<BraveWallet.AssetPriceTimeframe>(getStoredPortfolioTimeframe)

  const {
    data: portfolioPriceHistory,
    isFetching: isFetchingPortfolioPriceHistory,
  } = useGetPricesHistoryQuery(
    !isCollectionView
      && visibleTokensForFilteredChains.length
      && tokenBalancesRegistry
      && defaultFiat
      && !hidePortfolioGraph
      ? {
          tokens: visibleTokensForFilteredChains,
          timeframe: selectedTimeframe,
          vsAsset: defaultFiat,
          tokenBalancesRegistry,
        }
      : skipToken,
  )

  const distributionData: PortfolioOverviewDistributionData[] =
    React.useMemo(() => {
      if (
        visibleAssetOptions.length === 0
        || fullPortfolioFiatBalance.isZero()
        || fullPortfolioFiatBalance.isUndefined()
        || !defaultFiat
      ) {
        return []
      }

      // Calculate fiat value for each asset
      const assetsWithFiat = visibleAssetOptions
        .map((item) => {
          const fiatAmount = computeFiatAmount({
            spotPrices,
            value: item.assetBalance,
            token: item.asset,
          })
          return {
            token: item.asset,
            fiatAmount,
          }
        })
        .filter((item) => !item.fiatAmount.isZero())

      // Sort by fiat value descending
      const sorted = [...assetsWithFiat].sort((a, b) =>
        b.fiatAmount.minus(a.fiatAmount).toNumber(),
      )

      // Take top 3
      const topDistributionAssets = sorted.slice(0, DISTRIBUTION_LIMIT)

      // Sum the rest for "Other"
      const otherAssets = sorted.slice(DISTRIBUTION_LIMIT)
      const otherTotal = otherAssets.reduce(
        (sum, item) => sum.plus(item.fiatAmount),
        Amount.zero(),
      )

      // Build distribution data
      const result: PortfolioOverviewDistributionData[] =
        topDistributionAssets.map((item) => ({
          kind: 'asset',
          token: item.token,
          value: parseFloat(
            item.fiatAmount.div(fullPortfolioFiatBalance).times(100).format(2),
          ),
          fiatValue: item.fiatAmount.compactAsFiat(defaultFiat),
        }))

      // Add "Other" if there are more than DISTRIBUTION_LIMIT
      if (otherAssets.length > 0) {
        result.push({
          kind: 'other',
          value: parseFloat(
            otherTotal.div(fullPortfolioFiatBalance).times(100).format(2),
          ),
          fiatValue: otherTotal.compactAsFiat(defaultFiat),
        })
      }

      return result
    }, [visibleAssetOptions, spotPrices, fullPortfolioFiatBalance, defaultFiat])

  // methods
  const onSelectAsset = React.useCallback(
    (asset: BraveWallet.BlockchainToken) => {
      history.push(makePortfolioAssetRoute(false, getAssetIdKey(asset)))
    },
    [history],
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
        spotPrices={spotPrices}
        renderToken={({ item, account }) => (
          <PortfolioAssetItem
            action={() => onSelectAsset(item.asset)}
            key={getAssetIdKey(item.asset)}
            assetBalance={
              !tokenBalancesRegistry
                ? ''
                : selectedGroupAssetsByItem === AccountsGroupByOption.id
                    && !getIsRewardsToken(item.asset)
                  ? getBalance(
                      account?.accountId,
                      item.asset,
                      tokenBalancesRegistry,
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
              spotPrices
                ? getTokenPriceAmountFromRegistry(
                    spotPrices,
                    item.asset,
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
    spotPrices,
    tokenBalancesRegistry,
  ])

  // Computed
  const hasZeroBalance = fullPortfolioFiatBalance.isZero()

  // render
  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      noCardPadding={true}
      cardHeader={<PortfolioOverviewHeader />}
      useDarkBackground={isMobileOrPanel}
      isPortfolio={true}
    >
      <DefaultPageWrapper>
        <Column
          fullWidth={true}
          padding={
            isSidePanel
              ? '20px 0px 0px 0px'
              : isMobileOrPanel
                ? '0px'
                : '20px 20px 0px 20px'
          }
        >
          <Banners />
        </Column>
        {!isCollectionView && (
          <>
            <BalanceAndLineChartWrapper
              fullWidth={true}
              justifyContent='flex-start'
            >
              <BalanceAndButtonsWrapper
                fullWidth={true}
                hasZeroBalance={hasZeroBalance}
              >
                <BalanceAndChangeWrapper hasZeroBalance={hasZeroBalance}>
                  {formattedFullPortfolioFiatBalance !== '' ? (
                    <LastPricesUpdatedTooltip>
                      <Text
                        variant='components.numbersLarge'
                        textColor='primary'
                      >
                        {hidePortfolioBalances
                          ? '******'
                          : formattedFullPortfolioFiatBalance}
                      </Text>
                    </LastPricesUpdatedTooltip>
                  ) : (
                    <Column padding='9px 0px'>
                      <LoadingSkeleton
                        width={150}
                        height={36}
                      />
                    </Column>
                  )}
                  {!hidePortfolioGraph && !hasZeroBalance && (
                    <PortfolioValueChange
                      fullPortfolioFiatBalance={fullPortfolioFiatBalance}
                      defaultFiat={defaultFiat}
                      tokens={visibleTokensForFilteredChains}
                      tokenBalancesRegistry={tokenBalancesRegistry}
                      timeframe={selectedTimeframe}
                      skip={isCollectionView}
                    />
                  )}
                </BalanceAndChangeWrapper>
                {!hasZeroBalance && <BuySendSwapDepositNav />}
              </BalanceAndButtonsWrapper>
              {hasZeroBalance && <GettingStarted />}
              {!hasZeroBalance && (
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
              )}
              {!hasZeroBalance && (
                <ColumnReveal hideContent={hidePortfolioDistribution}>
                  <PortfolioOverviewDistribution data={distributionData} />
                </ColumnReveal>
              )}
            </BalanceAndLineChartWrapper>
            <ControlsRow>
              <SegmentedControl
                navOptions={
                  hidePortfolioNFTsTab
                    ? PortfolioNavOptionsNoNFTsTab
                    : PortfolioNavOptions
                }
                maxWidth='384px'
              />
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
            path={WalletRoutes.PortfolioActivity}
            exact
          >
            <ActivityWrapper
              fullWidth={true}
              fullHeight={true}
              justifyContent='flex-start'
              isMobileOrPanel={isMobileOrPanel}
            >
              <TransactionsScreen isPortfolio={true} />
            </ActivityWrapper>
          </Route>
        </Switch>

        {showPortfolioSettings && (
          <PortfolioFiltersModal
            onSave={() => {
              // reset to first page after filters change
              const newParams = new URLSearchParams(location.search)
              newParams.delete('page')
              history.push({
                ...location,
                search: `?${newParams.toString()}`,
              })
            }}
            onClose={() => {
              setShowPortfolioSettings(false)
            }}
          />
        )}
      </DefaultPageWrapper>
    </WalletPageWrapper>
  )
}

export default PortfolioOverview
