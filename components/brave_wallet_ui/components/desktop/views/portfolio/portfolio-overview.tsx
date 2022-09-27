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
import { getBalance } from '../../../../utils/balance-utils'
import { computeFiatAmount } from '../../../../utils/pricing-utils'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'
import { AllNetworksOption } from '../../../../options/network-filter-options'
import { AllAccountsOption } from '../../../../options/account-filter-options'

// Components
import { LoadingSkeleton } from '../../../shared'
import {
  ChartControlBar,
  LineChart,
  PortfolioAssetItem,
  WithHideBalancePlaceholder
} from '../../'
import { NFTGridViewItem } from './components/nft-grid-view/nft-grid-view-item'
import { TokenLists } from './components/token-lists/token-list'

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
  const defaultCurrencies = useSelector(({ wallet }: { wallet: WalletState }) => wallet.defaultCurrencies)
  const userVisibleTokensInfo = useSelector(({ wallet }: { wallet: WalletState }) => wallet.userVisibleTokensInfo)
  const portfolioPriceHistory = useSelector(({ wallet }: { wallet: WalletState }) => wallet.portfolioPriceHistory)
  const selectedPortfolioTimeline = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedPortfolioTimeline)
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)
  const networkList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)
  const isFetchingPortfolioPriceHistory = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isFetchingPortfolioPriceHistory)
  const transactionSpotPrices = useSelector(({ wallet }: { wallet: WalletState }) => wallet.transactionSpotPrices)
  const selectedNetworkFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter)
  const selectedAccountFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedAccountFilter)
  const selectedTimeline = useSelector(({ page }: { page: PageState }) => page.selectedTimeline)
  const nftMetadata = useSelector(({ page }: { page: PageState }) => page.nftMetadata)

  // memos / computed

  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const tokensCoinType = getTokensCoinType(networkList, asset)
    const amounts = accounts
      .filter((account) => account.coin === tokensCoinType)
      .map((account) => getBalance(networkList, account, asset))

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
        getTokensCoinType(networkList, token) === selectedNetworkFilter.coin
      )
    }
    // Filter by all other assets by chainId's
    return userVisibleTokensInfo.filter((token) => token.chainId === selectedNetworkFilter.chainId)
  }, [
    selectedNetworkFilter.chainId,
    selectedNetworkFilter.coin,
    userVisibleTokensInfo,
    networkList
  ])

  // Filters visibleTokensForSupportedChains if a selectedAccountFilter is selected.
  const visibleTokensForFilteredAccount: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    return selectedAccountFilter.id === AllAccountsOption.id
      ? visibleTokensForSupportedChains
      : visibleTokensForSupportedChains.filter((token) => token.coin === selectedAccountFilter.coin)
  }, [visibleTokensForSupportedChains, selectedAccountFilter])

  // This looks at the users asset list and returns the full balance for each asset
  const userAssetList: UserAssetInfoType[] = React.useMemo(() => {
    return visibleTokensForFilteredAccount.map((asset) => ({
      asset: asset,
      assetBalance: selectedAccountFilter.id === AllAccountsOption.id
        ? fullAssetBalance(asset)
        : getBalance(networkList, selectedAccountFilter, asset)
    }))
  }, [visibleTokensForFilteredAccount, selectedAccountFilter, networkList, fullAssetBalance])

  const visibleAssetOptions = React.useMemo((): UserAssetInfoType[] => {
    return userAssetList.filter(({ asset }) => asset.visible && !asset.isErc721)
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
            symbol: item.asset.symbol
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

  const priceHistory = React.useMemo((): PriceDataObjectType[] => {
    if (isZeroBalance) {
      return []
    }
    return portfolioPriceHistory
  }, [portfolioPriceHistory, isZeroBalance])

  // state
  const [hoverBalance, setHoverBalance] = React.useState<string>()
  const [hideBalances, setHideBalances] = React.useState<boolean>(false)

  // methods
  const onChangeTimeline = React.useCallback((timeline: BraveWallet.AssetPriceTimeframe) => {
    dispatch(WalletActions.selectPortfolioTimeline(timeline))
  }, [])

  const onSelectAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => {
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
  }, [defaultCurrencies.fiat])

  const onToggleHideBalances = React.useCallback(() => {
    setHideBalances(prev => !prev)
  }, [])

  // effects
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
        userAssetList={userAssetList}
        networks={networkList}
        estimatedItemSize={58}
        renderToken={({ item, viewMode }) =>
          viewMode === 'list'
          ? <PortfolioAssetItem
              spotPrices={transactionSpotPrices}
              defaultCurrencies={defaultCurrencies}
              action={() => onSelectAsset(item.asset)}
              key={
                !item.asset.isErc721
                  ? `${item.asset.contractAddress}-${item.asset.symbol}-${item.asset.chainId}`
                  : `${item.asset.contractAddress}-${item.asset.tokenId}-${item.asset.chainId}`
              }
              assetBalance={item.assetBalance}
              token={item.asset}
              hideBalances={hideBalances}
              networks={networkList}
            />
          : <NFTGridViewItem
              key={`${item.asset.tokenId}-${item.asset.contractAddress}`}
              token={item}
              onSelectAsset={() => onSelectAsset(item.asset)}
            />
        }
      />
    </StyledWrapper>
  )
}

export default PortfolioOverview
