// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { Redirect, useHistory, useParams } from 'react-router'

// types
import {
  BraveWallet,
  UserAssetInfoType,
  AddAccountNavTypes,
  WalletState,
  PageState,
  SupportedTestNetworks,
  WalletRoutes
} from '../../../../constants/types'

// Utils
import Amount from '../../../../utils/amount'
import { mojoTimeDeltaToJSDate } from '../../../../../common/mojomUtils'
import { sortTransactionByDate } from '../../../../utils/tx-utils'
import { getTokensNetwork, getTokensCoinType } from '../../../../utils/network-utils'

// actions
import { WalletPageActions } from '../../../../page/actions'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'
import { AllNetworksOption } from '../../../../options/network-filter-options'

// Components
import { BackButton, withPlaceholderIcon } from '../../../shared'
import {
  ChartControlBar,
  LineChart
} from '../../'
// import NFTDetails from './components/nft-details'
import AccountsAndTransactionsList from './components/accounts-and-transctions-list'

// Hooks
import { useBalance, usePricing, useTransactionParser } from '../../../../common/hooks'

// Styled Components
import {
  StyledWrapper,
  TopRow,
  AssetIcon,
  AssetRow,
  AssetColumn,
  PriceRow,
  AssetNameText,
  DetailText,
  InfoColumn,
  PriceText,
  PercentBubble,
  PercentText,
  ArrowIcon,
  BalanceRow,
  ShowBalanceButton,
  NetworkDescription
} from './style'
import { Skeleton } from '../../../shared/loading-skeleton/styles'
import NFTDetails from './components/nft-details'

const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 12 })

export const PortfolioAsset = () => {
  // routing
  const history = useHistory()
  const { id: assetId } = useParams<{ id?: string }>()

  // redux
  const dispatch = useDispatch()
  const {
    defaultCurrencies,
    userVisibleTokensInfo,
    selectedNetwork,
    portfolioPriceHistory,
    selectedPortfolioTimeline,
    accounts,
    networkList,
    transactions,
    isFetchingPortfolioPriceHistory,
    transactionSpotPrices,
    selectedNetworkFilter
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const {
    isFetchingPriceHistory: isLoading,
    selectedAsset,
    selectedAssetCryptoPrice,
    selectedAssetFiatPrice,
    selectedAssetPriceHistory,
    selectedTimeline,
    isFetchingNFTMetadata,
    nftMetadata
  } = useSelector(({ page }: { page: PageState }) => page)

  // custom hooks
  const getAccountBalance = useBalance(networkList)

  // memos
  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const tokensCoinType = getTokensCoinType(networkList, asset)
    const amounts = accounts.filter((account) => account.coin === tokensCoinType).map((account) =>
      getAccountBalance(account, asset))

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
  }, [accounts, networkList, getAccountBalance])

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

  // state
  const [filteredAssetList, setfilteredAssetList] = React.useState<UserAssetInfoType[]>(userAssetList)
  const [hoverPrice, setHoverPrice] = React.useState<string>()
  const [hideBalances, setHideBalances] = React.useState<boolean>(false)
  const selectedAssetsNetwork = React.useMemo(() => {
    if (!selectedAsset) {
      return selectedNetwork
    }
    return getTokensNetwork(networkList, selectedAsset)
  }, [selectedNetwork, selectedAsset, networkList])

  // more custom hooks
  const parseTransaction = useTransactionParser(selectedAssetsNetwork)
  const { computeFiatAmount } = usePricing(transactionSpotPrices)

  // memos / computed
  const selectedAssetFromParams = React.useMemo(() => {
    if (!assetId) {
      return undefined
    }

    // If the id length is greater than 15 assumes it's a contractAddress
    return assetId.length > 15
      ? userVisibleTokensInfo.find((token) => token.contractAddress === assetId)
      : userVisibleTokensInfo.find((token) => token.symbol.toLowerCase() === assetId?.toLowerCase())
  }, [assetId, userVisibleTokensInfo, selectedTimeline])

  // This will scrape all of the user's accounts and combine the fiat value for every asset
  const fullPortfolioFiatBalance = React.useMemo(() => {
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
    return grandTotal.formatAsFiat()
  }, [userAssetList, computeFiatAmount])

  const formattedPriceHistory = React.useMemo(() => {
    return selectedAssetPriceHistory.map((obj) => {
      return {
        date: mojoTimeDeltaToJSDate(obj.date),
        close: Number(obj.price)
      }
    })
  }, [selectedAssetPriceHistory])

  const priceHistory = React.useMemo(() => {
    if (parseFloat(fullPortfolioFiatBalance) === 0) {
      return []
    } else {
      return portfolioPriceHistory
    }
  }, [portfolioPriceHistory, fullPortfolioFiatBalance])

  const transactionsByNetwork = React.useMemo(() => {
    const accountsByNetwork = accounts.filter((account) => account.coin === selectedAssetsNetwork.coin)
    return accountsByNetwork.map((account) => {
      return transactions[account.address]
    }).flat(1)
  }, [accounts, transactions, selectedAssetsNetwork])

  const selectedAssetTransactions = React.useMemo((): BraveWallet.TransactionInfo[] => {
    if (selectedAsset) {
      const filteredTransactions = transactionsByNetwork.filter((tx) => {
        return tx && parseTransaction(tx).symbol === selectedAsset?.symbol
      })
      return sortTransactionByDate(filteredTransactions, 'descending')
    }
    return []
  }, [selectedAsset, transactionsByNetwork, parseTransaction])

  const fullAssetBalances = React.useMemo(() => {
    if (selectedAsset?.contractAddress === '') {
      return filteredAssetList.find(
        (asset) =>
          asset.asset.symbol.toLowerCase() === selectedAsset?.symbol.toLowerCase() &&
          asset.asset.chainId === selectedAsset?.chainId
      )
    }
    return filteredAssetList.find(
      (asset) =>
        asset.asset.contractAddress.toLowerCase() === selectedAsset?.contractAddress.toLowerCase() &&
        asset.asset.chainId === selectedAsset?.chainId
    )
  }, [filteredAssetList, selectedAsset])

  const fullAssetFiatBalance = React.useMemo(() => fullAssetBalances?.assetBalance
    ? computeFiatAmount(
        fullAssetBalances.assetBalance,
        fullAssetBalances.asset.symbol,
        fullAssetBalances.asset.decimals
      )
    : Amount.empty(),
    [fullAssetBalances]
  )

  const formattedFullAssetBalance = fullAssetBalances?.assetBalance
    ? '(' + new Amount(fullAssetBalances?.assetBalance ?? '')
      .divideByDecimals(selectedAsset?.decimals ?? 18)
      .formatAsAsset(6, selectedAsset?.symbol ?? '') + ')'
    : ''

  const isNftAsset = selectedAssetFromParams?.isErc721

  // methods
  const onClickAddAccount = React.useCallback((tabId: AddAccountNavTypes) => () => {
    history.push(WalletRoutes.AddAccountModal)
  }, [])

  const onChangeTimeline = React.useCallback((timeline: BraveWallet.AssetPriceTimeframe) => {
    dispatch(WalletPageActions.selectAsset({
      asset: selectedAsset,
      timeFrame: timeline
    }))
  }, [selectedAsset])

  const goBack = React.useCallback(() => {
    dispatch(WalletPageActions.selectAsset({ asset: undefined, timeFrame: selectedTimeline }))
    history.push(WalletRoutes.Portfolio)
    setfilteredAssetList(userAssetList)
  }, [
    userAssetList,
    selectedTimeline
  ])

  const onUpdateBalance = React.useCallback((value: number | undefined) => {
    setHoverPrice(value ? new Amount(value).formatAsFiat(defaultCurrencies.fiat) : undefined)
  }, [defaultCurrencies])

  const onToggleHideBalances = React.useCallback(() => {
    setHideBalances(prevHideBalances => !prevHideBalances)
  }, [])

  // effects
  React.useEffect(() => {
    setfilteredAssetList(userAssetList)
  }, [userAssetList])

  React.useEffect(() => {
    if (selectedAssetFromParams) {
      // load token data
      dispatch(WalletPageActions.selectAsset({ asset: selectedAssetFromParams, timeFrame: selectedTimeline }))
    }
  }, [selectedAssetFromParams])

  // token list needs to load before we can find an asset to select from the url params
  if (userVisibleTokensInfo.length === 0) {
    return <Skeleton />
  }

  // asset not found
  if (!selectedAssetFromParams) {
    return <Redirect to={WalletRoutes.Portfolio} />
  }

  // render
  return (
    <StyledWrapper>
      <TopRow>
        <BalanceRow>
          <BackButton onSubmit={goBack} />
        </BalanceRow>
        <BalanceRow>
          {!isNftAsset &&
            <ChartControlBar
              onSubmit={onChangeTimeline}
              selectedTimeline={selectedAsset ? selectedTimeline : selectedPortfolioTimeline}
              timelineOptions={ChartTimelineOptions()}
            />
          }
          <ShowBalanceButton
            hideBalances={hideBalances}
            onClick={onToggleHideBalances}
          />
        </BalanceRow>
      </TopRow>

      {!isNftAsset &&
        <InfoColumn>

          <AssetRow>
            <AssetIconWithPlaceholder asset={selectedAsset} network={selectedAssetsNetwork} />
            <AssetColumn>
              <AssetNameText>{selectedAssetFromParams.name}</AssetNameText>
              <NetworkDescription>{selectedAssetFromParams.symbol} on {selectedAssetsNetwork?.chainName ?? ''}</NetworkDescription>
            </AssetColumn>
          </AssetRow>

          <PriceRow>
            <PriceText>{hoverPrice || (selectedAssetFiatPrice ? new Amount(selectedAssetFiatPrice.price).formatAsFiat(defaultCurrencies.fiat) : 0.00)}</PriceText>
            <PercentBubble isDown={selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0 : false}>
              <ArrowIcon isDown={selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0 : false} />
              <PercentText>{selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange).toFixed(2) : 0.00}%</PercentText>
            </PercentBubble>
          </PriceRow>

          <DetailText>
            {
              selectedAssetCryptoPrice
                ? new Amount(selectedAssetCryptoPrice.price)
                  .formatAsAsset(undefined, defaultCurrencies.crypto)
                : ''
            }
          </DetailText>

        </InfoColumn>
      }

      {!isNftAsset &&
        <LineChart
          isDown={selectedAsset && selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0 : false}
          isAsset={!!selectedAsset}
          priceData={
            selectedAsset
              ? formattedPriceHistory
              : priceHistory
          }
          onUpdateBalance={onUpdateBalance}
          isLoading={selectedAsset ? isLoading : parseFloat(fullPortfolioFiatBalance) === 0 ? false : isFetchingPortfolioPriceHistory}
          isDisabled={selectedAsset ? false : parseFloat(fullPortfolioFiatBalance) === 0}
        />
      }

      {selectedAsset?.isErc721 &&
        <NFTDetails
          isLoading={isFetchingNFTMetadata}
          selectedAsset={selectedAsset}
          nftMetadata={nftMetadata}
          defaultCurrencies={defaultCurrencies}
          selectedNetwork={selectedNetwork}
        />
      }

      <AccountsAndTransactionsList
        formattedFullAssetBalance={formattedFullAssetBalance}
        fullAssetFiatBalance={fullAssetFiatBalance}
        selectedAsset={selectedAsset}
        selectedAssetTransactions={selectedAssetTransactions}
        onClickAddAccount={onClickAddAccount}
        hideBalances={hideBalances}
        networkList={networkList}
      />
    </StyledWrapper>
  )
}

export default PortfolioAsset
