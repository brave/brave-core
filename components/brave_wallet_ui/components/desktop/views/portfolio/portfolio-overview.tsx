// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory } from 'react-router'

// selectors
import {
  useSafeWalletSelector,
  useUnsafeWalletSelector,
  useSafePageSelector,
  useUnsafePageSelector
} from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'
import { PageSelectors } from '../../../../page/selectors'

// Constants
import {
  BraveWallet,
  UserAssetInfoType,
  SupportedTestNetworks,
  WalletRoutes
} from '../../../../constants/types'

// actions
import { WalletActions } from '../../../../common/actions'
import { WalletPageActions } from '../../../../page/actions'

// Utils
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'
import { getBalance } from '../../../../utils/balance-utils'
import { computeFiatAmount } from '../../../../utils/pricing-utils'
import { getAssetIdKey } from '../../../../utils/asset-utils'
import { formatAsDouble } from '../../../../utils/string-utils'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'
import { AllNetworksOption } from '../../../../options/network-filter-options'
import { AllAccountsOption } from '../../../../options/account-filter-options'

// Components
import { LoadingSkeleton } from '../../../shared'
import { PortfolioAssetItem } from '../../'
import { NFTGridViewItem } from './components/nft-grid-view/nft-grid-view-item'
import { TokenLists } from './components/token-lists/token-list'
import {
  PortfolioOverviewChart //
} from './components/portfolio-overview-chart/portfolio-overview-chart'
import { ChartControlBar } from '../../chart-control-bar/chart-control-bar'
import ColumnReveal from '../../../shared/animated-reveals/column-reveal'

// Styled Components
import {
  BalanceTitle,
  BalanceText,
  BalanceRow
} from './style'
import {
  Column,
  Row,
  VerticalSpace
} from '../../../shared/style'
import { PlaceholderText } from '../../with-hide-balance-placeholder/style'
import { useGetVisibleNetworksQuery } from '../../../../common/slices/api.slice'

export const PortfolioOverview = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  const defaultCurrencies =
    useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)
  const userVisibleTokensInfo =
    useUnsafeWalletSelector(WalletSelectors.userVisibleTokensInfo)
  const selectedPortfolioTimeline =
    useSafeWalletSelector(WalletSelectors.selectedPortfolioTimeline)
  const accounts =
    useUnsafeWalletSelector(WalletSelectors.accounts)
  const transactionSpotPrices =
    useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
  const selectedNetworkFilter =
    useUnsafeWalletSelector(WalletSelectors.selectedNetworkFilter)
  const selectedAccountFilter =
    useSafeWalletSelector(WalletSelectors.selectedAccountFilter)
  const selectedTimeline =
    useSafePageSelector(PageSelectors.selectedTimeline)
  const nftMetadata =
    useUnsafePageSelector(PageSelectors.nftMetadata)
  const hidePortfolioGraph =
    useSafeWalletSelector(WalletSelectors.hidePortfolioGraph)
  const hidePortfolioBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioBalances)


  // queries
  const { data: networks } = useGetVisibleNetworksQuery()

  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const amounts = accounts
      .filter((account) => account.coin === asset.coin)
      .map((account) => getBalance(account, asset))

    // If a user has not yet created a FIL or SOL account,
    // we return 0 until they create an account
    if (amounts.length === 0) {
      return '0'
    }

    return amounts.reduce(function (a, b) {
      return a !== '' && b !== ''
        ? new Amount(a).plus(b).format()
        : ''
    })
  }, [accounts, getBalance])

  // memos / computed

  // filter the user's assets based on the selected network
  const visibleTokensForSupportedChains = React.useMemo(() => {
    // By default we dont show any testnetwork assets
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return userVisibleTokensInfo.filter((token) => !SupportedTestNetworks.includes(token.chainId))
    }

    // If chainId is Localhost we also do a check for coinType to return
    // the correct asset
    if (selectedNetworkFilter.chainId === BraveWallet.LOCALHOST_CHAIN_ID) {
      return userVisibleTokensInfo.filter((token) =>
        token.chainId === selectedNetworkFilter.chainId &&
        token.coin === selectedNetworkFilter.coin
      )
    }
    // Filter by all other assets by chainId's
    return userVisibleTokensInfo.filter((token) => token.chainId === selectedNetworkFilter.chainId)
  }, [
    selectedNetworkFilter.chainId,
    selectedNetworkFilter.coin,
    userVisibleTokensInfo
  ])

  // Filters visibleTokensForSupportedChains if a selectedAccountFilter is selected.
  const visibleTokensForFilteredAccount: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    return selectedAccountFilter === AllAccountsOption.id
      ? visibleTokensForSupportedChains
      : visibleTokensForSupportedChains.filter((token) => token.coin === accounts.find(account => account.id === selectedAccountFilter)?.coin)
  }, [visibleTokensForSupportedChains, selectedAccountFilter, accounts])

  // This looks at the users asset list and returns the full balance for each asset
  const userAssetList: UserAssetInfoType[] = React.useMemo(() => {
    return visibleTokensForFilteredAccount.map((asset) => ({
      asset: asset,
      assetBalance: selectedAccountFilter === AllAccountsOption.id
        ? fullAssetBalance(asset)
        : getBalance(accounts.find(account => account.id === selectedAccountFilter), asset)
    }))
  }, [
    visibleTokensForFilteredAccount,
    selectedAccountFilter,
    fullAssetBalance,
    accounts
  ])

  const visibleAssetOptions = React.useMemo((): UserAssetInfoType[] => {
    return userAssetList.filter(({ asset }) => asset.visible && !(asset.isErc721 || asset.isNft))
  }, [userAssetList])

  // This will scrape all of the user's accounts and combine the fiat value for every asset
  const fullPortfolioFiatBalance = React.useMemo((): string => {
    if (visibleAssetOptions.length === 0) {
      return ''
    }

    const visibleAssetFiatBalances = visibleAssetOptions
      .map((item) => {
        return computeFiatAmount(
          transactionSpotPrices,
          {
            value: item.assetBalance,
            decimals: item.asset.decimals,
            symbol: item.asset.symbol,
            contractAddress: item.asset.contractAddress,
            chainId: item.asset.chainId
          }
        )
      })

    const grandTotal = visibleAssetFiatBalances.reduce(function (a, b) {
      return a.plus(b)
    })
    return grandTotal.formatAsFiat(defaultCurrencies.fiat)
  },
    [
      visibleAssetOptions,
      defaultCurrencies.fiat,
      transactionSpotPrices
    ])

  const isZeroBalance = React.useMemo((): boolean => {
    // In some cases we need to check if the balance is zero
    return parseFloat(formatAsDouble(fullPortfolioFiatBalance)) === 0
  }, [fullPortfolioFiatBalance])


  // methods
  const onChangeTimeline = React.useCallback((timeline: BraveWallet.AssetPriceTimeframe) => {
    dispatch(WalletActions.selectPortfolioTimeline(timeline))
  }, [])

  const onSelectAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    if (asset.contractAddress === '') {
      history.push(`${WalletRoutes.Portfolio}/${asset.chainId}/${asset.symbol}`)
      return
    }
    history.push(`${WalletRoutes.Portfolio}/${asset.chainId}/${asset.contractAddress}/${asset.tokenId}`)
    dispatch(WalletPageActions.selectAsset({ asset, timeFrame: selectedTimeline }))
    if ((asset.isErc721 || asset.isNft) && nftMetadata) {
      // reset nft metadata
      dispatch(WalletPageActions.updateNFTMetadata(undefined))
    }
  }, [selectedTimeline])

  // effects
  React.useEffect(() => {
    dispatch(WalletPageActions.selectAsset({ asset: undefined, timeFrame: selectedTimeline }))
  }, [selectedTimeline])

  // render
  return (
    <>
      <Column fullWidth={true} justifyContent='flex-start'>
        <Row alignItems='flex-start' justifyContent='flex-start'>
          <BalanceTitle>{getLocale('braveWalletBalance')}</BalanceTitle>
        </Row>

        <Row justifyContent='space-between'>
          <Column>
            <BalanceRow>
              {hidePortfolioBalances
                ? <PlaceholderText isBig>******</PlaceholderText>
                : <div>
                  <BalanceText>
                    {fullPortfolioFiatBalance !== ''
                      ? fullPortfolioFiatBalance
                      : <LoadingSkeleton width={150} height={32} />
                    }
                  </BalanceText>
                </div>
              }
            </BalanceRow>
          </Column>

          <Column>
            <BalanceRow>
              <ChartControlBar
                disabled={hidePortfolioGraph}
                onSelectTimeframe={onChangeTimeline}
                selectedTimeline={selectedPortfolioTimeline}
                timelineOptions={ChartTimelineOptions}
              />
            </BalanceRow>
          </Column>
        </Row>

        <VerticalSpace space='20px' />

        <ColumnReveal hideContent={hidePortfolioGraph}>
          <PortfolioOverviewChart
            hasZeroBalance={isZeroBalance}
          />
        </ColumnReveal>
      </Column>
      <TokenLists
        userAssetList={userAssetList}
        networks={networks || []}
        estimatedItemSize={58}
        enableScroll={true}
        renderToken={({ item, viewMode }) =>
          viewMode === 'list'
            ? <PortfolioAssetItem
              action={() => onSelectAsset(item.asset)}
              key={getAssetIdKey(item.asset)}
              assetBalance={item.assetBalance}
              token={item.asset}
              hideBalances={hidePortfolioBalances}
            />
            : <NFTGridViewItem
              key={`${item.asset.tokenId}-${item.asset.contractAddress}`}
              token={item.asset}
              onSelectAsset={() => onSelectAsset(item.asset)}
            />
        }
      />

    </>
  )
}

export default PortfolioOverview
