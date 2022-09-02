// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { Redirect, useHistory, useParams } from 'react-router'

// types
import {
  AddAccountNavTypes,
  BraveWallet,
  PageState,
  SupportedTestNetworks,
  UserAssetInfoType,
  WalletRoutes,
  WalletState
} from '../../../../constants/types'

// Utils
import Amount from '../../../../utils/amount'
import { mojoTimeDeltaToJSDate } from '../../../../../common/mojomUtils'
import { sortTransactionByDate } from '../../../../utils/tx-utils'
import { getTokensCoinType, getTokensNetwork } from '../../../../utils/network-utils'
import useExplorer from '../../../../common/hooks/explorer'
import {
  CommandMessage,
  NftUiCommand,
  sendMessageToNftUiFrame,
  ToggleNftModal,
  UpdateLoadingMessage,
  UpdateNFtMetadataMessage,
  UpdateSelectedAssetMessage,
  UpdateTokenNetworkMessage,
  braveNftDisplayOrigin
} from '../../../../nft/nft-ui-messages'
import { auroraSupportedContractAddresses } from '../../../../utils/asset-utils'
import { getLocale } from '../../../../../common/locale'
// actions
import { WalletPageActions } from '../../../../page/actions'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'
import { AllNetworksOption } from '../../../../options/network-filter-options'

// Components
import { BackButton } from '../../../shared'
import withPlaceholderIcon from '../../../shared/create-placeholder-icon'
import { ChartControlBar, LineChart } from '../../'
import AccountsAndTransactionsList from './components/accounts-and-transctions-list'
import { BridgeToAuroraModal } from '../../popup-modals/bridge-to-aurora-modal/bridge-to-aurora-modal'

// Hooks
import { useBalance, usePricing, useTransactionParser } from '../../../../common/hooks'

// Styled Components
import {
  ArrowIcon,
  AssetColumn,
  AssetIcon,
  AssetNameText,
  AssetRow,
  BalanceRow,
  BridgeToAuroraButton,
  DetailText,
  InfoColumn,
  NetworkDescription,
  NftDetails,
  PercentBubble,
  PercentText,
  PriceRow,
  PriceText,
  ShowBalanceButton,
  StyledWrapper,
  TopRow,
  SubDivider,
  NotSupportedText,
  MoreButton
} from './style'
import { Skeleton } from '../../../shared/loading-skeleton/styles'
import { CoinStats } from './components/coin-stats/coin-stats'
import { AssetMorePopup } from './components/asset-more-popup/asset-more-popup'
import { TokenDetailsModal } from './components/token-details-modal/token-details-modal'
import { WalletActions } from '../../../../common/actions'
import { HideTokenModal } from './components/hide-token-modal/hide-token-modal'
import { NftModal } from './components/nft-modal/nft-modal'

const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 12 })
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
  const [isTokenSupported, setIsTokenSupported] = React.useState<boolean>()
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [showTokenDetailsModal, setShowTokenDetailsModal] = React.useState<boolean>(false)
  const [showHideTokenModel, setShowHideTokenModal] = React.useState<boolean>(false)
  const [showNftModal, setshowNftModal] = React.useState<boolean>(false)

  // routing
  const history = useHistory()
  const { id: assetId, tokenId } = useParams<{ id?: string, tokenId?: string }>()
  const nftDetailsRef = React.useRef<HTMLIFrameElement>(null)
  const [nftIframeLoaded, setNftIframeLoaded] = React.useState(false)
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
    selectedNetworkFilter,
    coinMarketData,
    fullTokenList
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const {
    isFetchingPriceHistory: isLoading,
    selectedAsset,
    selectedAssetCryptoPrice,
    selectedAssetFiatPrice,
    selectedAssetPriceHistory,
    selectedTimeline,
    isFetchingNFTMetadata,
    nftMetadata,
    selectedCoinMarket
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

  const tokensWithCoingeckoId = React.useMemo(() => {
    return fullTokenList.filter(token => token.coingeckoId !== '')
  }, [fullTokenList])

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
  const openExplorer = useExplorer(selectedAssetsNetwork)

  // memos / computed
  const selectedAssetFromParams = React.useMemo(() => {
    if (!assetId) {
      return undefined
    }

    // If the id length is greater than 15 assumes it's a contractAddress
    let token = assetId.length > 15
      ? userVisibleTokensInfo.find((token) => tokenId ? token.contractAddress === assetId && token.tokenId === tokenId : token.contractAddress === assetId)
      : userVisibleTokensInfo.find((token) => token.symbol.toLowerCase() === assetId?.toLowerCase())

    if (!token && assetId.length < 15) {
      const coinMarket = coinMarketData.find(token => token.symbol.toLowerCase() === assetId?.toLowerCase())
      if (coinMarket) {
        token = new BraveWallet.BlockchainToken()
        token.coingeckoId = coinMarket.id
        token.name = coinMarket.name
        token.contractAddress = ''
        token.symbol = coinMarket.symbol.toUpperCase()
        token.logo = coinMarket.image
      }
      const foundToken = tokensWithCoingeckoId?.find(token => token.coingeckoId.toLowerCase() === coinMarket?.id?.toLowerCase())
      setIsTokenSupported(foundToken !== undefined)
    } else {
      setIsTokenSupported(true)
    }

    return token
  }, [assetId, userVisibleTokensInfo, selectedTimeline, tokenId])

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

  const formattedAssetBalance = React.useMemo(() => {
    if (!fullAssetBalances?.assetBalance) return ''

    return new Amount(fullAssetBalances.assetBalance)
      .divideByDecimals(selectedAsset?.decimals ?? 18)
      .formatAsAsset()
  }, [fullAssetBalances, selectedAsset])

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
    dispatch(WalletPageActions.selectCoinMarket(undefined))
    setfilteredAssetList(userAssetList)
    if (isShowingMarketData) {
      history.push(WalletRoutes.Market)
    } else {
      history.push(WalletRoutes.Portfolio)
    }
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

  const onNftDetailsLoad = React.useCallback(() => setNftIframeLoaded(true), [])

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

  const onShowMore = React.useCallback((e: React.MouseEvent<HTMLButtonElement>) => {
    e.stopPropagation()
    setShowMore(true)
  }, [])

  const onHideMore = React.useCallback((e: React.MouseEvent<HTMLDivElement>) => {
    e.stopPropagation()
    setShowMore(false)
  }, [])

  const onShowTokenDetailsModal = React.useCallback(() => setShowTokenDetailsModal(true), [])

  const onCloseTokenDetailsModal = React.useCallback(() => setShowTokenDetailsModal(false), [])

  const onShowHideTokenModal = React.useCallback(() => setShowHideTokenModal(true), [])

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
    history.push(WalletRoutes.Portfolio)
  }, [selectedAsset, showTokenDetailsModal])

  const onViewOnExplorer = React.useCallback(() => {
    if (selectedAsset) {
      openExplorer('token', selectedAsset.contractAddress)()
    }
  }, [selectedAsset])

  const onCloseNftModal = React.useCallback(() => {
    setshowNftModal(false)
  }, [])

  const onMessageEventListener = React.useCallback((event: MessageEvent<CommandMessage>) => {
    // validate message origin
    if (event.origin !== braveNftDisplayOrigin) return

    const message = event.data
    if (message.command === NftUiCommand.ToggleNftModal) {
      const { payload } = message as ToggleNftModal
      setshowNftModal(payload)
    }
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

  React.useEffect(() => {
    if (!nftIframeLoaded) return

    if (nftDetailsRef?.current) {
      const command: UpdateLoadingMessage = {
        command: NftUiCommand.UpdateLoading,
        payload: isFetchingNFTMetadata
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }
  }, [nftIframeLoaded, nftDetailsRef, isFetchingNFTMetadata])

  React.useEffect(() => {
    if (!nftIframeLoaded) return

    if (selectedAsset && nftDetailsRef?.current) {
      const command: UpdateSelectedAssetMessage = {
        command: NftUiCommand.UpdateSelectedAsset,
        payload: selectedAsset
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }

    if (selectedAsset && networkList && nftDetailsRef?.current) {
      const tokenNetwork = getTokensNetwork(networkList, selectedAsset)
      const command: UpdateTokenNetworkMessage = {
        command: NftUiCommand.UpdateTokenNetwork,
        payload: tokenNetwork
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }

    if (nftMetadata && nftDetailsRef?.current) {
      const command: UpdateNFtMetadataMessage = {
        command: NftUiCommand.UpdateNFTMetadata,
        payload: nftMetadata
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }
  }, [nftIframeLoaded, nftDetailsRef, selectedAsset, nftMetadata, networkList])

  React.useEffect(() => {
    setDontShowAuroraWarning(JSON.parse(localStorage.getItem(bridgeToAuroraDontShowAgainKey) || 'false'))
  })

  // Receive postMessage from chrome-untrusted://nft-display
  React.useEffect(() => {
    window.addEventListener('message', onMessageEventListener)
    return () => window.removeEventListener('message', onMessageEventListener)
  }, [onMessageEventListener])

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
    <StyledWrapper onClick={onHideMore}>
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
          {selectedAsset?.contractAddress && !selectedAsset?.isErc721 &&
            <MoreButton onClick={onShowMore} />
          }
          {showMore && selectedAsset &&
            <AssetMorePopup
              assetSymbol={selectedAsset.symbol}
              onClickTokenDetails={onShowTokenDetailsModal}
              onClickViewOnExplorer={onViewOnExplorer}
              onClickHideToken={onShowHideTokenModal}
            />
          }
        </BalanceRow>
      </TopRow>

      {!isNftAsset &&
        <InfoColumn>

          <AssetRow>
            <AssetIconWithPlaceholder asset={selectedAsset} network={selectedAssetsNetwork} />
            <AssetColumn>
              <AssetNameText>{selectedAssetFromParams.name}</AssetNameText>
              <NetworkDescription>{selectedAssetFromParams.symbol} {selectedAssetsNetwork?.chainName && `on ${selectedAssetsNetwork?.chainName}`}</NetworkDescription>
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

      {!isNftAsset && isSelectedAssetBridgeSupported &&
        <BridgeToAuroraButton
          onClick={onBridgeToAuroraButton}
        >
          {getLocale('braveWalletBridgeToAuroraButton')}
        </BridgeToAuroraButton>
      }

      {showBridgeToAuroraModal &&
        <BridgeToAuroraModal
          dontShowWarningAgain={dontShowAuroraWarning}
          onClose={onCloseAuroraModal}
          onOpenRainbowAppClick={onOpenRainbowAppClick}
          onDontShowAgain={onDontShowAgain}
        />
      }

      {showTokenDetailsModal && selectedAsset && selectedAssetsNetwork &&
        <TokenDetailsModal
          onClose={onCloseTokenDetailsModal}
          selectedAsset={selectedAsset}
          selectedAssetNetwork={selectedAssetsNetwork}
          assetBalance={formattedAssetBalance}
          formattedFiatBalance={fullAssetFiatBalance.formatAsFiat(defaultCurrencies.fiat)}
          onShowHideTokenModal={onShowHideTokenModal}
        />
      }

      {showHideTokenModel && selectedAsset && selectedAssetsNetwork &&
        <HideTokenModal
          selectedAsset={selectedAsset}
          selectedAssetNetwork={selectedAssetsNetwork}
          onClose={onCloseHideTokenModal}
          onHideAsset={onHideAsset}
        />
      }

      <NftDetails
        onLoad={onNftDetailsLoad}
        visible={selectedAsset?.isErc721}
        ref={nftDetailsRef}
        sandbox="allow-scripts allow-popups allow-same-origin"
        src='chrome-untrusted://nft-display'
      />

      {showNftModal && nftMetadata &&
        <NftModal nftImageUrl={nftMetadata.imageURL} onClose={onCloseNftModal} />
      }

      {isTokenSupported
        ? <AccountsAndTransactionsList
          formattedFullAssetBalance={formattedFullAssetBalance}
          fullAssetFiatBalance={fullAssetFiatBalance}
          selectedAsset={selectedAsset}
          selectedAssetTransactions={selectedAssetTransactions}
          onClickAddAccount={onClickAddAccount}
          hideBalances={hideBalances}
          networkList={networkList}
        />
        : <>
          <SubDivider />
          <NotSupportedText>{getLocale('braveWalletMarketDataCoinNotSupported')}</NotSupportedText>
        </>
      }

      {isShowingMarketData && selectedCoinMarket &&
        <CoinStats
          marketCapRank={selectedCoinMarket.marketCapRank}
          volume={selectedCoinMarket.totalVolume}
          marketCap={selectedCoinMarket.marketCap}
        />
      }
    </StyledWrapper>
  )
}

export default PortfolioAsset
