// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory } from 'react-router'

// Constants
import {
  BraveWallet,
  UserAssetInfoType,
  WalletState,
  PageState,
  SupportedTestNetworks,
  WalletRoutes,
  PriceDataObjectType
} from '../../../../constants/types'
import { getLocale } from '../../../../../common/locale'

// Utils
import { getTokensCoinType } from '../../../../utils/network-utils'
import { formatAsDouble } from '../../../../utils/string-utils'
import Amount from '../../../../utils/amount'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'
import { AllNetworksOption } from '../../../../options/network-filter-options'

// Components
import { LoadingSkeleton } from '../../../shared'
import {
  ChartControlBar,
  LineChart,
  WithHideBalancePlaceholder
} from '../../'

// import NFTDetails from './components/nft-details'
import TokenLists from './components/token-lists'

// Hooks
import { useBalance, usePricing } from '../../../../common/hooks'

// Styled Components
import {
  StyledWrapper,
  TopRow,
  BalanceTitle,
  BalanceText,
  BalanceRow,
  ShowBalanceButton
} from './style'

// actions
import { WalletActions } from '../../../../common/actions'
import { WalletPageActions } from '../../../../page/actions'

export const PortfolioOverview = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const {
    defaultCurrencies,
    userVisibleTokensInfo,
    portfolioPriceHistory,
    selectedPortfolioTimeline,
    accounts,
    networkList,
    isFetchingPortfolioPriceHistory,
    transactionSpotPrices,
    selectedNetworkFilter
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const { selectedTimeline, nftMetadata } = useSelector(({ page }: { page: PageState }) => page)

  // custom hooks
  const getAccountAssetBalance = useBalance(networkList)
  const { computeFiatAmount } = usePricing(transactionSpotPrices)

  // memos / computed

  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const tokensCoinType = getTokensCoinType(networkList, asset)
    const amounts = accounts
      .filter((account) => account.coin === tokensCoinType)
      .map((account) => getAccountAssetBalance(account, asset))

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
  }, [accounts, networkList, getAccountAssetBalance])

  // This looks at the users asset list and returns the full balance for each asset
  const userAssetList = React.useMemo(() => {
    const allAssets = userVisibleTokensInfo.map((asset) => ({
      asset: asset,
      assetBalance: fullAssetBalance(asset)
    }) as UserAssetInfoType)
    // By default we dont show any testnetwork assets
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return allAssets.filter((asset) => !SupportedTestNetworks.includes(asset.asset.chainId))
    }
    // If chainId is Localhost we also do a check for coinType to return
    // the correct asset
    if (selectedNetworkFilter.chainId === BraveWallet.LOCALHOST_CHAIN_ID) {
      return allAssets.filter((asset) =>
        asset.asset.chainId === selectedNetworkFilter.chainId &&
        getTokensCoinType(networkList, asset.asset) === selectedNetworkFilter.coin
      )
    }
    // Filter by all other assets by chainId's
    return allAssets.filter((asset) => asset.asset.chainId === selectedNetworkFilter.chainId)
  }, [userVisibleTokensInfo, selectedNetworkFilter, fullAssetBalance, networkList])

  // This will scrape all of the user's accounts and combine the fiat value for every asset
  const fullPortfolioFiatBalance = React.useMemo((): string => {
    const visibleAssetOptions = userAssetList
      .filter((token) =>
        token.asset.visible &&
        !token.asset.isErc721
      )

    if (visibleAssetOptions.length === 0) {
      return ''
    }

    const visibleAssetFiatBalances = visibleAssetOptions
      .map((item) => {
        return computeFiatAmount(item.assetBalance, item.asset.symbol, item.asset.decimals)
      })

    const grandTotal = visibleAssetFiatBalances.reduce(function (a, b) {
      return a.plus(b)
    })
    return grandTotal.formatAsFiat(defaultCurrencies.fiat)
  }, [userAssetList, defaultCurrencies, computeFiatAmount])

  const isZeroBalance = React.useMemo((): boolean => {
    // In some cases we need to check if the balance is zero
    return parseFloat(formatAsDouble(fullPortfolioFiatBalance)) === 0
  }, [fullPortfolioFiatBalance])

  const priceHistory = React.useMemo((): PriceDataObjectType[] => {
    if (isZeroBalance) {
      return []
    }
    return portfolioPriceHistory
  }, [portfolioPriceHistory, isZeroBalance])

  // state
  const [filteredAssetList, setfilteredAssetList] = React.useState<UserAssetInfoType[]>(userAssetList)
  const [hoverBalance, setHoverBalance] = React.useState<string>()
  const [hideBalances, setHideBalances] = React.useState<boolean>(false)

  // methods
  const onChangeTimeline = React.useCallback((timeline: BraveWallet.AssetPriceTimeframe) => {
    dispatch(WalletActions.selectPortfolioTimeline(timeline))
  }, [])

  const onSelectAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => () => {
    if (asset.contractAddress === '') {
      history.push(`${WalletRoutes.Portfolio}/${asset.symbol}`)
      return
    } else if (asset.isErc721) {
      history.push(`${WalletRoutes.Portfolio}/${asset.contractAddress}/${asset.tokenId}`)
    } else {
      history.push(`${WalletRoutes.Portfolio}/${asset.contractAddress}`)
    }

    dispatch(WalletPageActions.selectAsset({ asset, timeFrame: selectedTimeline }))
    if (asset.isErc721 && nftMetadata) {
      // reset nft metadata
      dispatch(WalletPageActions.updateNFTMetadata(undefined))
    }
  }, [selectedTimeline])

  const onUpdateBalance = React.useCallback((value: number | undefined) => {
    setHoverBalance(value ? new Amount(value).formatAsFiat(defaultCurrencies.fiat) : undefined)
  }, [defaultCurrencies])

  const onToggleHideBalances = React.useCallback(() => {
    setHideBalances(!hideBalances)
  }, [hideBalances])

  // effects
  React.useEffect(() => {
    setfilteredAssetList(userAssetList)
  }, [userAssetList])

  React.useEffect(() => {
    dispatch(WalletPageActions.selectAsset({ asset: undefined, timeFrame: selectedTimeline }))
  }, [selectedTimeline])

  // render
  return (
    <StyledWrapper>
      <TopRow>
        <BalanceRow>
          <BalanceTitle>{getLocale('braveWalletBalance')}</BalanceTitle>
        </BalanceRow>
        <BalanceRow>
          <ChartControlBar
            onSubmit={onChangeTimeline}
            selectedTimeline={selectedPortfolioTimeline}
            timelineOptions={ChartTimelineOptions()}
          />
          <ShowBalanceButton
            hideBalances={hideBalances}
            onClick={onToggleHideBalances}
          />
        </BalanceRow>
      </TopRow>

      <WithHideBalancePlaceholder
        size='big'
        hideBalances={hideBalances}
      >
        <BalanceText>
          {fullPortfolioFiatBalance !== ''
            ? `${hoverBalance || fullPortfolioFiatBalance}`
            : <LoadingSkeleton width={150} height={32} />
          }
        </BalanceText>
      </WithHideBalancePlaceholder>

      <LineChart
        isDown={false}
        isAsset={false}
        priceData={priceHistory}
        onUpdateBalance={onUpdateBalance}
        isLoading={isZeroBalance ? false : isFetchingPortfolioPriceHistory}
        isDisabled={isZeroBalance}
      />

      <TokenLists
        defaultCurrencies={defaultCurrencies}
        userAssetList={userAssetList}
        filteredAssetList={filteredAssetList}
        tokenPrices={transactionSpotPrices}
        networks={networkList}
        onSetFilteredAssetList={setfilteredAssetList}
        onSelectAsset={onSelectAsset}
        hideBalances={hideBalances}
      />
    </StyledWrapper>
  )
}

export default PortfolioOverview
