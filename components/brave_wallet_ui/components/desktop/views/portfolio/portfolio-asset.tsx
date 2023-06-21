// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { Redirect, useHistory, useParams } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import {
  AddAccountNavTypes,
  BraveWallet,
  SupportedTestNetworks,
  UserAssetInfoType,
  WalletRoutes
} from '../../../../constants/types'

// Utils
import Amount from '../../../../utils/amount'
import { mojoTimeDeltaToJSDate } from '../../../../../common/mojomUtils'
import {
  findTransactionToken,
  getETHSwapTransactionBuyAndSellTokens,
  sortTransactionByDate
} from '../../../../utils/tx-utils'
import { getBalance } from '../../../../utils/balance-utils'
import { computeFiatAmount } from '../../../../utils/pricing-utils'
import { getPriceIdForToken } from '../../../../utils/api-utils'
import {
  auroraSupportedContractAddresses,
  getAssetIdKey
} from '../../../../utils/asset-utils'
import { getLocale } from '../../../../../common/locale'
import { makeNetworkAsset } from '../../../../options/asset-options'

// actions
import { WalletPageActions } from '../../../../page/actions'

// selectors
import { WalletSelectors } from '../../../../common/selectors'
import { PageSelectors } from '../../../../page/selectors'

// Options
import { AllNetworksOption } from '../../../../options/network-filter-options'

// Components
import { LineChart } from '../../'
import {
  LineChartControls
} from '../../line-chart/line-chart-controls/line-chart-controls'
import AccountsAndTransactionsList from './components/accounts-and-transctions-list'
import { BridgeToAuroraModal } from '../../popup-modals/bridge-to-aurora-modal/bridge-to-aurora-modal'
import { NftScreen } from '../../../../nft/components/nft-details/nft-screen'

// Hooks
import { useMultiChainBuyAssets } from '../../../../common/hooks'
import {
  useSafePageSelector,
  useSafeWalletSelector,
  useUnsafePageSelector,
  useUnsafeWalletSelector
} from '../../../../common/hooks/use-safe-selector'
import {
  useGetNetworkQuery,
  useGetSelectedChainQuery,
  useGetTransactionsQuery,
  useGetTokenSpotPricesQuery
} from '../../../../common/slices/api.slice'
import {
  useGetCombinedTokensListQuery
} from '../../../../common/slices/api.slice.extra'
import {
  querySubscriptionOptions60s
} from '../../../../common/slices/constants'

// Styled Components
import {
  BridgeToAuroraButton,
  StyledWrapper,
  ButtonRow
} from './style'
import { Row, Column } from '../../../shared/style'
import { Skeleton } from '../../../shared/loading-skeleton/styles'
import { CoinStats } from './components/coin-stats/coin-stats'
import { TokenDetailsModal } from './components/token-details-modal/token-details-modal'
import { WalletActions } from '../../../../common/actions'
import { HideTokenModal } from './components/hide-token-modal/hide-token-modal'
import {
  WalletPageWrapper
} from '../../wallet-page-wrapper/wallet-page-wrapper'
import {
  AssetDetailsHeader
} from '../../card-headers/asset-details-header'
import NftAssetHeader from '../../card-headers/nft-asset-header'

const rainbowbridgeLink = 'https://rainbowbridge.app'
const bridgeToAuroraDontShowAgainKey = 'bridgeToAuroraDontShowAgain'

interface Props {
  isShowingMarketData?: boolean
}

export const PortfolioAsset = (props: Props) => {
  const { isShowingMarketData } = props
  // state
  const [showBridgeToAuroraModal, setShowBridgeToAuroraModal] = React.useState<boolean>(false)
  const [dontShowAuroraWarning, setDontShowAuroraWarning] = React.useState<boolean>(false)
  const [showTokenDetailsModal, setShowTokenDetailsModal] = React.useState<boolean>(false)
  const [showHideTokenModel, setShowHideTokenModal] = React.useState<boolean>(false)

  // routing
  const history = useHistory()
  const { chainIdOrMarketSymbol, contractOrSymbol, tokenId } = useParams<{ chainIdOrMarketSymbol?: string, contractOrSymbol?: string, tokenId?: string }>()

  // redux
  const dispatch = useDispatch()

  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)
  const userVisibleTokensInfo = useUnsafeWalletSelector(WalletSelectors.userVisibleTokensInfo)
  const portfolioPriceHistory = useUnsafeWalletSelector(WalletSelectors.portfolioPriceHistory)
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const isFetchingPortfolioPriceHistory = useSafeWalletSelector(WalletSelectors.isFetchingPortfolioPriceHistory)
  const selectedNetworkFilter = useUnsafeWalletSelector(WalletSelectors.selectedNetworkFilter)
  const coinMarketData = useUnsafeWalletSelector(WalletSelectors.coinMarketData)

  const isLoading = useSafePageSelector(PageSelectors.isFetchingPriceHistory)
  const selectedAsset = useUnsafePageSelector(PageSelectors.selectedAsset)
  const selectedAssetPriceHistory = useUnsafePageSelector(PageSelectors.selectedAssetPriceHistory)
  const selectedTimeline = useSafePageSelector(PageSelectors.selectedTimeline)
  const selectedCoinMarket = useUnsafePageSelector(PageSelectors.selectedCoinMarket)
  const hiddenNfts =
    useUnsafeWalletSelector(WalletSelectors.removedNonFungibleTokens)

  // queries
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()
  const { data: assetsNetwork } = useGetNetworkQuery(
    selectedAsset ?? skipToken //
  )
  const { data: selectedNetwork } = useGetSelectedChainQuery(undefined, {
    skip: !!assetsNetwork
  })
  const selectedAssetsNetwork = assetsNetwork || selectedNetwork

  const { data: transactionsByNetwork = [] } = useGetTransactionsQuery(
    selectedAsset
      ? {
        address: null,
        chainId: selectedAsset.chainId,
        coinType: selectedAsset.coin
      }
      : skipToken
  )

  // custom hooks
  const { allAssetOptions, isReduxSelectedAssetBuySupported, getAllBuyOptionsAllChains } = useMultiChainBuyAssets()

  // memos
  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const amounts = accounts.filter((account) => account.accountId.coin === asset.coin).map((account) =>
      getBalance(account, asset))

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
  }, [accounts])

  // This looks at the users asset list and returns the full balance for each asset
  const userAssetList = React.useMemo(() => {
    const allAssets = userVisibleTokensInfo.map(
      (asset) =>
      ({
        asset: asset,
        assetBalance: fullAssetBalance(asset)
      } as UserAssetInfoType)
    )
    // By default we dont show any testnetwork assets
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return allAssets.filter(
        (asset) => !SupportedTestNetworks.includes(asset.asset.chainId)
      )
    }
    // If chainId is Localhost we also do a check for coinType to return
    // the correct asset
    if (selectedNetworkFilter.chainId === BraveWallet.LOCALHOST_CHAIN_ID) {
      return allAssets.filter(
        (asset) =>
          asset.asset.chainId === selectedNetworkFilter.chainId &&
          asset.asset.coin === selectedNetworkFilter.coin
      )
    }
    // Filter by all other assets by chainId's
    return allAssets.filter(
      (asset) => asset.asset.chainId === selectedNetworkFilter.chainId
    )
  }, [
    userVisibleTokensInfo,
    selectedNetworkFilter.chainId,
    selectedNetworkFilter.coin,
    fullAssetBalance
  ])

  // state
  const [filteredAssetList, setfilteredAssetList] = React.useState<UserAssetInfoType[]>(userAssetList)

  // memos / computed
  const selectedAssetFromParams = React.useMemo(() => {
    if (!chainIdOrMarketSymbol) {
      return undefined
    }

    if (isShowingMarketData) {
      const coinMarket = coinMarketData.find(token => token.symbol.toLowerCase() === chainIdOrMarketSymbol.toLowerCase())
      let token = undefined as BraveWallet.BlockchainToken | undefined
      if (coinMarket) {
        token = new BraveWallet.BlockchainToken()
        token.coingeckoId = coinMarket.id
        token.name = coinMarket.name
        token.contractAddress = ''
        token.symbol = coinMarket.symbol.toUpperCase()
        token.logo = coinMarket.image
      }
      return token
    }
    if (!contractOrSymbol) {
      return undefined
    }
    const userToken = [...userVisibleTokensInfo, ...hiddenNfts]
    .find((token) =>
      tokenId
        ? token.tokenId === tokenId && token.contractAddress.toLowerCase() === contractOrSymbol.toLowerCase() && token.chainId === chainIdOrMarketSymbol
        : token.contractAddress.toLowerCase() === contractOrSymbol.toLowerCase() && token.chainId === chainIdOrMarketSymbol ||
        token.symbol.toLowerCase() === contractOrSymbol.toLowerCase() && token.chainId === chainIdOrMarketSymbol && token.contractAddress === '')
    return userToken
  }, [
    userVisibleTokensInfo,
    selectedTimeline,
    chainIdOrMarketSymbol,
    contractOrSymbol,
    tokenId,
    isShowingMarketData,
    hiddenNfts
  ])

  const tokenPriceIds = React.useMemo(() =>
    userAssetList
      .filter(({ assetBalance }) => new Amount(assetBalance).gt(0))
      .filter(({ asset }) =>
        !asset.isErc721 && !asset.isErc1155 && !asset.isNft)
      .map(({ asset }) => getPriceIdForToken(asset))
      .concat(
        selectedAssetFromParams
          ? [getPriceIdForToken(selectedAssetFromParams)]
          : []
    ),
    [userAssetList, selectedAssetFromParams]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length ? { ids: tokenPriceIds } : skipToken,
    querySubscriptionOptions60s
  )

  const isSelectedAssetBridgeSupported = React.useMemo(() => {
    if (!selectedAssetFromParams) return false
    const isBridgeAddress = auroraSupportedContractAddresses.includes(selectedAssetFromParams.contractAddress.toLowerCase())
    const isNativeAsset = selectedAssetFromParams.contractAddress === ''

    return (isBridgeAddress || isNativeAsset) && selectedAssetFromParams.chainId === BraveWallet.MAINNET_CHAIN_ID
  }, [selectedAssetFromParams])

  // This will scrape all of the user's accounts and combine the fiat value for every asset
  const fullPortfolioFiatBalance = React.useMemo(() => {
    const visibleAssetOptions = userAssetList
      .filter((token) =>
        token.asset.visible &&
        !(token.asset.isErc721 || token.asset.isNft)
      )

    if (visibleAssetOptions.length === 0) {
      return ''
    }

    const visibleAssetFiatBalances = visibleAssetOptions
      .map((item) => {
        return computeFiatAmount({
          spotPriceRegistry,
          value: item.assetBalance,
          token: item.asset
        })
      })

    const grandTotal = visibleAssetFiatBalances.reduce(function (a, b) {
      return a.plus(b)
    })
    return grandTotal.formatAsFiat()
  }, [userAssetList, spotPriceRegistry])

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

  const selectedAssetTransactions = React.useMemo(() => {
    if (selectedAsset) {
      const filteredTransactions = transactionsByNetwork.filter((tx) => {
        const token = findTransactionToken(tx, [selectedAsset])

        const { sellToken, buyToken } = getETHSwapTransactionBuyAndSellTokens({
          nativeAsset: makeNetworkAsset(selectedAssetsNetwork),
          tokensList: [selectedAsset],
          tx
        })

        return (
          (token && getAssetIdKey(selectedAsset) === getAssetIdKey(token)) ||
          (sellToken &&
            getAssetIdKey(selectedAsset) === getAssetIdKey(sellToken)) ||
          (buyToken && getAssetIdKey(selectedAsset) === getAssetIdKey(buyToken))
        )
      })
      return sortTransactionByDate(filteredTransactions, 'descending')
    }
    return []
  }, [
    selectedAsset,
    transactionsByNetwork,
    selectedAssetsNetwork
  ])

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
    ? computeFiatAmount({
      spotPriceRegistry,
      value: fullAssetBalances.assetBalance,
      token: fullAssetBalances.asset
    })
    : Amount.empty(),
    [fullAssetBalances, spotPriceRegistry]
  )

  const formattedFullAssetBalance = fullAssetBalances?.assetBalance
    ? '(' + new Amount(fullAssetBalances?.assetBalance ?? '')
      .divideByDecimals(selectedAsset?.decimals ?? 18)
      .formatAsAsset(6, selectedAsset?.symbol ?? '') + ')'
    : ''

  const formattedAssetBalance = React.useMemo(() => {
    if (!fullAssetBalances?.assetBalance) return ''

    return new Amount(fullAssetBalances.assetBalance)
      .divideByDecimals(selectedAsset?.decimals ?? 18)
      .formatAsAsset(8)
  }, [fullAssetBalances, selectedAsset])

  const isNftAsset = selectedAssetFromParams?.isErc721 || selectedAssetFromParams?.isNft

  const isSelectedAssetDepositSupported = React.useMemo(() => {
    if (!selectedAsset) {
      return false
    }

    return combinedTokensList.some(token =>
      token.symbol.toLowerCase() === selectedAsset.symbol.toLowerCase())
  }, [combinedTokensList, selectedAsset?.symbol])

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
    dispatch(WalletPageActions.selectCoinMarket(undefined))
    dispatch(WalletPageActions.updateNFTMetadata(undefined))
    dispatch(WalletPageActions.updateNftMetadataError(undefined))
    setfilteredAssetList(userAssetList)
    if (isShowingMarketData) {
      history.push(WalletRoutes.Market)
      return
    }
    if (isNftAsset) {
      history.push(WalletRoutes.PortfolioNFTs)
      return
    }
    history.push(WalletRoutes.PortfolioAssets)
  }, [
    isShowingMarketData,
    isNftAsset,
    userAssetList,
    selectedTimeline
  ])

  const onOpenRainbowAppClick = React.useCallback(() => {
    chrome.tabs.create({ url: rainbowbridgeLink }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
    setShowBridgeToAuroraModal(false)
  }, [])

  const onBridgeToAuroraButton = React.useCallback(() => {
    if (dontShowAuroraWarning) {
      onOpenRainbowAppClick()
    } else {
      setShowBridgeToAuroraModal(true)
    }
  }, [dontShowAuroraWarning, onOpenRainbowAppClick])

  const onDontShowAgain = React.useCallback((selected: boolean) => {
    setDontShowAuroraWarning(selected)
    localStorage.setItem(bridgeToAuroraDontShowAgainKey, JSON.stringify(selected))
  }, [])

  const onCloseAuroraModal = React.useCallback(() => {
    setShowBridgeToAuroraModal(false)
  }, [])

  const onCloseTokenDetailsModal = React.useCallback(() => setShowTokenDetailsModal(false), [])

  const onCloseHideTokenModal = React.useCallback(() => setShowHideTokenModal(false), [])

  const onHideAsset = React.useCallback(() => {
    if (!selectedAsset) return
    dispatch(WalletActions.setUserAssetVisible({ token: selectedAsset, isVisible: false }))
    dispatch(WalletActions.refreshBalancesAndPriceHistory())
    dispatch(WalletPageActions.selectAsset({
      asset: undefined,
      timeFrame: BraveWallet.AssetPriceTimeframe.OneDay
    }))
    if (showHideTokenModel) setShowHideTokenModal(false)
    if (showTokenDetailsModal) setShowTokenDetailsModal(false)
    history.push(WalletRoutes.PortfolioAssets)
  }, [selectedAsset, showTokenDetailsModal])

  const onSelectBuy = React.useCallback(() => {
    history.push(`${WalletRoutes.FundWalletPageStart}/${selectedAsset?.symbol}`)
  }, [selectedAsset?.symbol])

  const onSelectDeposit = React.useCallback(() => {
    history.push(`${WalletRoutes.DepositFundsPageStart}/${selectedAsset?.symbol}`)
  }, [selectedAsset?.symbol])

  const onSend = React.useCallback(() => {
    if (!selectedAsset || !selectedAssetsNetwork) return

    const account = accounts
      .filter((account) => account.accountId.coin === selectedAsset.coin)
      .find(acc => new Amount(getBalance(acc, selectedAsset)).gte('1'))

    if(!account) return

    history.push(
      WalletRoutes.SendPage.replace(':chainId?', selectedAssetsNetwork.chainId)
        .replace(':accountAddress?', account.address)
        .replace(':contractAddress?', selectedAsset.contractAddress)
        .replace(':tokenId?', selectedAsset.tokenId)
    )
  }, [selectedAsset, accounts, selectedAssetsNetwork])

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

  React.useEffect(() => {
    setDontShowAuroraWarning(JSON.parse(localStorage.getItem(bridgeToAuroraDontShowAgainKey) || 'false'))
  }, [])

  React.useEffect(() => {
    if (allAssetOptions.length === 0) {
      getAllBuyOptionsAllChains()
    }
  }, [allAssetOptions.length])

  // token list needs to load before we can find an asset to select from the url params
  if (userVisibleTokensInfo.length === 0) {
    return <Skeleton />
  }

  // asset not found
  if (!selectedAssetFromParams) {
    return <Redirect to={WalletRoutes.PortfolioAssets} />
  }

  // render
  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      noCardPadding={!isNftAsset}
      hideDivider={!isNftAsset}
      cardHeader={
        !isNftAsset
          ? <AssetDetailsHeader
            isShowingMarketData={isShowingMarketData}
            onBack={goBack}
            onClickTokenDetails={
              () => setShowTokenDetailsModal(true)
            }
            onClickHideToken={
              () => setShowHideTokenModal(true)
            }
          />
          : <NftAssetHeader
            onBack={goBack}  
            assetName={selectedAsset?.name}
            tokenId={selectedAsset?.tokenId}
            showSendButton={!new Amount(fullAssetBalances?.assetBalance || '').isZero()}
            onSend={onSend}
          />
      }
    >
      <StyledWrapper>
        {!isNftAsset &&
          <Row
            margin='20px 0px 8px 0px'
          >
            <LineChartControls
              onSelectTimeline={onChangeTimeline}
              selectedTimeline={selectedTimeline}
            />
          </Row>
        }

        {!isNftAsset &&
          <LineChart
            priceData={
              selectedAsset
                ? formattedPriceHistory
                : priceHistory
            }
            isLoading={
              selectedAsset
                ? isLoading
                : parseFloat(fullPortfolioFiatBalance) === 0
                  ? false
                  : isFetchingPortfolioPriceHistory
            }
            isDisabled={
              selectedAsset
                ? false
                : parseFloat(fullPortfolioFiatBalance) === 0
            }
          />
        }
        {!isNftAsset &&
          <Row
            padding='0px 20px'
          >
            <ButtonRow>
              {isReduxSelectedAssetBuySupported &&
                <BridgeToAuroraButton
                  onClick={onSelectBuy}
                  noBottomMargin={true}
                >
                  {getLocale('braveWalletBuy')}
                </BridgeToAuroraButton>
              }
              {isSelectedAssetDepositSupported &&
                <BridgeToAuroraButton
                  onClick={onSelectDeposit}
                  noBottomMargin={true}
                >
                  {getLocale('braveWalletAccountsDeposit')}
                </BridgeToAuroraButton>
              }
              {isSelectedAssetBridgeSupported &&
                <BridgeToAuroraButton
                  onClick={onBridgeToAuroraButton}
                  noBottomMargin={true}
                >
                  {getLocale('braveWalletBridgeToAuroraButton')}
                </BridgeToAuroraButton>
              }
            </ButtonRow>
          </Row>
        }

        {showBridgeToAuroraModal &&
          <BridgeToAuroraModal
            dontShowWarningAgain={dontShowAuroraWarning}
            onClose={onCloseAuroraModal}
            onOpenRainbowAppClick={onOpenRainbowAppClick}
            onDontShowAgain={onDontShowAgain}
          />
        }

        {showTokenDetailsModal &&
          selectedAsset &&
          selectedAssetsNetwork &&
          <TokenDetailsModal
            onClose={onCloseTokenDetailsModal}
            selectedAsset={selectedAsset}
            selectedAssetNetwork={selectedAssetsNetwork}
            assetBalance={formattedAssetBalance}
            formattedFiatBalance={
              fullAssetFiatBalance.formatAsFiat(defaultCurrencies.fiat)
            }
            onShowHideTokenModal={
              () => setShowHideTokenModal(true)
            }
          />
        }

        {showHideTokenModel &&
          selectedAsset &&
          selectedAssetsNetwork &&
          <HideTokenModal
            selectedAsset={selectedAsset}
            selectedAssetNetwork={selectedAssetsNetwork}
            onClose={onCloseHideTokenModal}
            onHideAsset={onHideAsset}
          />
        }

         {isNftAsset && selectedAsset &&
            <NftScreen
              selectedAsset={selectedAsset}
              tokenNetwork={selectedAssetsNetwork}
            />
          }

        {!isShowingMarketData && !isNftAsset &&
          <Column
            padding='0px 20px 20px 20px'
            fullWidth={true}
          >
            <AccountsAndTransactionsList
              formattedFullAssetBalance={formattedFullAssetBalance}
              fullAssetFiatBalance={fullAssetFiatBalance}
              selectedAsset={selectedAsset}
              selectedAssetTransactions={selectedAssetTransactions}
              onClickAddAccount={onClickAddAccount}
            />
          </Column>
        }

        {isShowingMarketData && selectedCoinMarket &&
          <Column
            padding='0px 20px 20px 20px'
            fullWidth={true}
          >
            <CoinStats
              marketCapRank={selectedCoinMarket.marketCapRank}
              volume={selectedCoinMarket.totalVolume}
              marketCap={selectedCoinMarket.marketCap}
            />
          </Column>
        }
      </StyledWrapper>
    </WalletPageWrapper>
  )
}

export default PortfolioAsset
