// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { Redirect, useHistory, useParams } from 'react-router'

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
  ParsedTransaction,
  sortTransactionByDate
} from '../../../../utils/tx-utils'
import { getBalance } from '../../../../utils/balance-utils'
import useExplorer from '../../../../common/hooks/explorer'
import {
  CommandMessage,
  NftUiCommand,
  sendMessageToNftUiFrame,
  ToggleNftModal,
  UpdateLoadingMessage,
  UpdateNFtMetadataErrorMessage,
  UpdateNFtMetadataMessage,
  UpdateSelectedAssetMessage,
  UpdateTokenNetworkMessage,
  braveNftDisplayOrigin,
  UpdateNftPinningStatus
} from '../../../../nft/nft-ui-messages'
import { auroraSupportedContractAddresses } from '../../../../utils/asset-utils'
import { getLocale } from '../../../../../common/locale'
import { stripERC20TokenImageURL } from '../../../../utils/string-utils'

// actions
import { WalletPageActions } from '../../../../page/actions'

// selectors
import { WalletSelectors } from '../../../../common/selectors'
import { PageSelectors } from '../../../../page/selectors'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'
import { AllNetworksOption } from '../../../../options/network-filter-options'

// Components
import { BackButton } from '../../../shared'
import withPlaceholderIcon from '../../../shared/create-placeholder-icon'
import { LineChart } from '../../'
import AccountsAndTransactionsList from './components/accounts-and-transctions-list'
import { BridgeToAuroraModal } from '../../popup-modals/bridge-to-aurora-modal/bridge-to-aurora-modal'

// Hooks
import { usePricing, useTransactionParser, useMultiChainBuyAssets } from '../../../../common/hooks'
import {
  useSafePageSelector,
  useSafeWalletSelector,
  useUnsafePageSelector,
  useUnsafeWalletSelector
} from '../../../../common/hooks/use-safe-selector'
import { useNftPin } from '../../../../common/hooks/nft-pin'
import {
  useGetNetworkQuery,
  useGetSelectedChainQuery
} from '../../../../common/slices/api.slice'

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
  NftMultimedia,
  PercentBubble,
  PercentText,
  PriceRow,
  PriceText,
  StyledWrapper,
  TopRow,
  MoreButton,
  ButtonRow
} from './style'
import { Skeleton } from '../../../shared/loading-skeleton/styles'
import { CoinStats } from './components/coin-stats/coin-stats'
import { AssetMorePopup } from './components/asset-more-popup/asset-more-popup'
import { TokenDetailsModal } from './components/token-details-modal/token-details-modal'
import { WalletActions } from '../../../../common/actions'
import { HideTokenModal } from './components/hide-token-modal/hide-token-modal'
import { NftModal } from './components/nft-modal/nft-modal'
import { ChartControlBar } from '../../chart-control-bar/chart-control-bar'
import { IpfsNodeStatus } from './components/ipfs-node-status/ipfs-node-status'
import {
  areSupportedForPinning,
  extractIpfsUrl
} from '../../../../common/async/lib'
import { ScrollableColumn } from '../../../shared/style'
import { NftDetails } from '../../../../nft/components/nft-details/nft-details'

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
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [showTokenDetailsModal, setShowTokenDetailsModal] = React.useState<boolean>(false)
  const [showHideTokenModel, setShowHideTokenModal] = React.useState<boolean>(false)
  const [showNftModal, setshowNftModal] = React.useState<boolean>(false)

  // routing
  const history = useHistory()
  const { chainIdOrMarketSymbol, contractOrSymbol, tokenId } = useParams<{ chainIdOrMarketSymbol?: string, contractOrSymbol?: string, tokenId?: string }>()
  const nftDetailsRef = React.useRef<HTMLIFrameElement>(null)
  const [nftIframeLoaded, setNftIframeLoaded] = React.useState(false)
  // redux
  const dispatch = useDispatch()

  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)
  const userVisibleTokensInfo = useUnsafeWalletSelector(WalletSelectors.userVisibleTokensInfo)
  const portfolioPriceHistory = useUnsafeWalletSelector(WalletSelectors.portfolioPriceHistory)
  const selectedPortfolioTimeline = useSafeWalletSelector(WalletSelectors.selectedPortfolioTimeline)
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const transactions = useUnsafeWalletSelector(WalletSelectors.transactions)
  const isFetchingPortfolioPriceHistory = useSafeWalletSelector(WalletSelectors.isFetchingPortfolioPriceHistory)
  const transactionSpotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
  const selectedNetworkFilter = useUnsafeWalletSelector(WalletSelectors.selectedNetworkFilter)
  const coinMarketData = useUnsafeWalletSelector(WalletSelectors.coinMarketData)
  const fullTokenList = useUnsafeWalletSelector(WalletSelectors.fullTokenList)

  const isLoading = useSafePageSelector(PageSelectors.isFetchingPriceHistory)
  const selectedAsset = useUnsafePageSelector(PageSelectors.selectedAsset)
  const selectedAssetCryptoPrice = useUnsafePageSelector(PageSelectors.selectedAssetCryptoPrice)
  const selectedAssetFiatPrice = useUnsafePageSelector(PageSelectors.selectedAssetFiatPrice)
  const selectedAssetPriceHistory = useUnsafePageSelector(PageSelectors.selectedAssetPriceHistory)
  const selectedTimeline = useSafePageSelector(PageSelectors.selectedTimeline)
  const isFetchingNFTMetadata = useSafePageSelector(PageSelectors.isFetchingNFTMetadata)
  const nftMetadata = useUnsafePageSelector(PageSelectors.nftMetadata)
  const selectedCoinMarket = useUnsafePageSelector(PageSelectors.selectedCoinMarket)
  const nftMetadataError = useSafePageSelector(PageSelectors.nftMetadataError)
  const nftPinningStatus = useUnsafePageSelector(PageSelectors.nftsPinningStatus)
  const isAutoPinEnabled = useSafePageSelector(PageSelectors.isAutoPinEnabled)

  // queries
  const { data: assetsNetwork } = useGetNetworkQuery(selectedAsset, {
    skip: !selectedAsset
  })
  const { data: selectedNetwork } = useGetSelectedChainQuery(undefined, {
    skip: !!assetsNetwork
  })
  const selectedAssetsNetwork = assetsNetwork || selectedNetwork

  // custom hooks
  const { allAssetOptions, isReduxSelectedAssetBuySupported, getAllBuyOptionsAllChains } = useMultiChainBuyAssets()
  const { getNftPinningStatus } = useNftPin()

  // memos
  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const amounts = accounts.filter((account) => account.coin === asset.coin).map((account) =>
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
  const [hoverPrice, setHoverPrice] = React.useState<string>()

  // more custom hooks
  const parseTransaction = useTransactionParser(selectedAssetsNetwork)
  const { computeFiatAmount } = usePricing(transactionSpotPrices)
  const openExplorer = useExplorer(selectedAssetsNetwork)

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
    const userToken = userVisibleTokensInfo.find((token) =>
      tokenId
        ? token.tokenId === tokenId && token.contractAddress.toLowerCase() === contractOrSymbol.toLowerCase() && token.chainId === chainIdOrMarketSymbol
        : token.contractAddress.toLowerCase() === contractOrSymbol.toLowerCase() && token.chainId === chainIdOrMarketSymbol ||
        token.symbol.toLowerCase() === contractOrSymbol.toLowerCase() && token.chainId === chainIdOrMarketSymbol && token.contractAddress === '')
    return userToken
  }, [userVisibleTokensInfo, selectedTimeline, chainIdOrMarketSymbol, contractOrSymbol, tokenId, isShowingMarketData])

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
        return computeFiatAmount(
          item.assetBalance,
          item.asset.symbol,
          item.asset.decimals,
          item.asset.contractAddress,
          item.asset.chainId
        )
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

  const accountsByNetwork = React.useMemo(() => {
    if (selectedAssetsNetwork?.coin !== undefined) {
      return []
    }
    return accounts.filter((account) => account.coin === selectedAssetsNetwork?.coin)
  }, [selectedAssetsNetwork?.coin, accounts])

  const transactionsByNetwork = React.useMemo(() => {
    return accountsByNetwork.map((account) => {
      return transactions[account.address]
    }).flat(1).map(parseTransaction)
  }, [
    accountsByNetwork,
    transactions
  ])

  const selectedAssetTransactions: ParsedTransaction[] = React.useMemo(() => {
    if (selectedAsset) {
      const filteredTransactions = transactionsByNetwork.filter((tx) => {
        return tx && tx?.symbol === selectedAsset?.symbol
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
      fullAssetBalances.asset.decimals,
      fullAssetBalances.asset.contractAddress,
      fullAssetBalances.asset.chainId
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
      .formatAsAsset(8)
  }, [fullAssetBalances, selectedAsset])

  const isNftAsset = selectedAssetFromParams?.isErc721 || selectedAssetFromParams?.isNft

  const isSelectedAssetDepositSupported = React.useMemo(() => {
    return fullTokenList.some((asset) => asset.symbol.toLowerCase() === selectedAsset?.symbol.toLowerCase())
  }, [fullTokenList, selectedAsset?.symbol])


  const isSelectedAssetPriceDown =
    selectedAsset &&
      selectedAssetFiatPrice
      ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0
      : false

  const networkDescription =
    isShowingMarketData
      ? selectedAssetFromParams?.symbol ?? ''
      : getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', selectedAssetFromParams?.symbol ?? '')
        .replace('$2', selectedAssetsNetwork?.chainName ?? '')


  const [ipfsImageUrl, setIpfsImageUrl] = React.useState<string>()
  const [nftPinnable, setNftPinnable] = React.useState<boolean>()

  React.useEffect(() => {
    let ignore = false
    if (nftMetadata?.imageURL) {
      areSupportedForPinning([nftMetadata?.imageURL]).then(
        (v) => {if (!ignore) setNftPinnable(v)})
      extractIpfsUrl(nftMetadata?.imageURL).then(
        (v) => {if (!ignore) setIpfsImageUrl(v)})
    }
    return () => {
      ignore = true
    }
  }, [nftMetadata])

  const currentNftPinningStatus = React.useMemo(() => {
    if (!isAutoPinEnabled) {
      return undefined
    }
    if (isNftAsset && selectedAsset && nftPinnable) {
      return getNftPinningStatus(selectedAsset)
    }
    return undefined
  }, [nftPinnable, isNftAsset, selectedAsset, nftPinningStatus, isAutoPinEnabled])

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
    history.goBack()
  }, [
    userAssetList,
    selectedTimeline
  ])

  const onUpdateBalance = React.useCallback((value: number | undefined) => {
    setHoverPrice(value ? new Amount(value).formatAsFiat(defaultCurrencies.fiat) : undefined)
  }, [defaultCurrencies.fiat])

  const onNftDetailsLoad = React.useCallback(() => {
    setNftIframeLoaded(true)
  }, [])

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
    if (
      fullTokenList.some((asset) =>
        asset.contractAddress.toLowerCase() ===
        selectedAsset.contractAddress.toLowerCase())
    ) {
      dispatch(WalletActions.removeUserAsset(selectedAsset))
    } else {
      dispatch(WalletActions.setUserAssetVisible({ token: selectedAsset, isVisible: false }))
    }
    dispatch(WalletActions.setUserAssetVisible({ token: selectedAsset, isVisible: false }))
    dispatch(WalletActions.refreshBalancesAndPriceHistory())
    dispatch(WalletPageActions.selectAsset({
      asset: undefined,
      timeFrame: BraveWallet.AssetPriceTimeframe.OneDay
    }))
    if (showHideTokenModel) setShowHideTokenModal(false)
    if (showTokenDetailsModal) setShowTokenDetailsModal(false)
    history.push(WalletRoutes.Portfolio)
  }, [selectedAsset, showTokenDetailsModal, fullTokenList])

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

  const onSelectBuy = React.useCallback(() => {
    history.push(`${WalletRoutes.FundWalletPageStart}/${selectedAsset?.symbol}`)
  }, [selectedAsset?.symbol])

  const onSelectDeposit = React.useCallback(() => {
    history.push(`${WalletRoutes.DepositFundsPageStart}/${selectedAsset?.symbol}`)
  }, [selectedAsset?.symbol])

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

    if (selectedAsset && selectedAssetsNetwork && nftDetailsRef?.current) {
      const command: UpdateTokenNetworkMessage = {
        command: NftUiCommand.UpdateTokenNetwork,
        payload: selectedAssetsNetwork
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }

    if (nftMetadata && nftDetailsRef?.current) {
      const command: UpdateNFtMetadataMessage = {
        command: NftUiCommand.UpdateNFTMetadata,
        payload: {
          displayMode: 'details',
          nftMetadata
        }
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }

    if (nftMetadataError && nftDetailsRef?.current) {
      const command: UpdateNFtMetadataErrorMessage = {
        command: NftUiCommand.UpdateNFTMetadataError,
        payload: {
          displayMode: 'details',
          error: nftMetadataError
        }
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }

    if (nftDetailsRef?.current) {
      const command: UpdateNftPinningStatus = {
        command: NftUiCommand.UpdateNftPinningStatus,
        payload: {
          status: currentNftPinningStatus,
          url: ipfsImageUrl
        }
      }
      sendMessageToNftUiFrame(nftDetailsRef.current.contentWindow, command)
    }

    // check if selectedAsset has an icon
    if (
      selectedAsset &&
      nftMetadata?.imageURL &&
      stripERC20TokenImageURL(selectedAsset.logo) === ''
    ) {
      // update asset logo
      const updated = { ...selectedAsset, logo: nftMetadata?.imageURL || '' }
      dispatch(
        WalletActions.updateUserAsset({
          existing: selectedAsset,
          updated
        })
      )
    }
  }, [
    currentNftPinningStatus,
    ipfsImageUrl,
    nftDetailsRef,
    nftIframeLoaded,
    nftMetadata,
    nftMetadataError,
    nftPinningStatus,
    selectedAsset,
    selectedAssetsNetwork
  ])

  React.useEffect(() => {
    setDontShowAuroraWarning(JSON.parse(localStorage.getItem(bridgeToAuroraDontShowAgainKey) || 'false'))
  }, [])

  // Receive postMessage from chrome-untrusted://nft-display
  React.useEffect(() => {
    window.addEventListener('message', onMessageEventListener)
    return () => window.removeEventListener('message', onMessageEventListener)
  }, [onMessageEventListener])

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
    return <Redirect to={WalletRoutes.Portfolio} />
  }

  // render
  return (
    <StyledWrapper onClick={onHideMore}>
      <TopRow>
        <BalanceRow gap='16px'>
          <BackButton onSubmit={goBack} />
          {isNftAsset && currentNftPinningStatus?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNED && <IpfsNodeStatus />}
        </BalanceRow>
        <BalanceRow>
          {!isNftAsset &&
            <ChartControlBar
              onSelectTimeframe={onChangeTimeline}
              selectedTimeline={selectedAsset ? selectedTimeline : selectedPortfolioTimeline}
              timelineOptions={ChartTimelineOptions}
            />
          }
          {selectedAsset?.contractAddress && !selectedAsset?.isErc721 && !selectedAsset.isNft &&
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

      <ScrollableColumn>
        {!isNftAsset &&
          <InfoColumn>

            <AssetRow>
              <AssetIconWithPlaceholder
                asset={selectedAsset}
                network={selectedAssetsNetwork}
              />
              <AssetColumn>
                <AssetNameText>{selectedAssetFromParams.name}</AssetNameText>
                <NetworkDescription>
                  {networkDescription}
                </NetworkDescription>
              </AssetColumn>
            </AssetRow>

            <PriceRow>
              <PriceText>
                {
                  hoverPrice ||
                  (
                    selectedAssetFiatPrice
                      ? new Amount(selectedAssetFiatPrice.price)
                        .formatAsFiat(defaultCurrencies.fiat)
                      : 0.00
                  )
                }
              </PriceText>
              <PercentBubble
                isDown={isSelectedAssetPriceDown}
              >
                <ArrowIcon
                  isDown={isSelectedAssetPriceDown}
                />
                <PercentText>
                  {
                    selectedAssetFiatPrice
                      ? Number(selectedAssetFiatPrice.assetTimeframeChange)
                        .toFixed(2)
                      : 0.00
                  }%
                </PercentText>
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
            isDown={isSelectedAssetPriceDown}
            isAsset={!!selectedAsset}
            priceData={
              selectedAsset
                ? formattedPriceHistory
                : priceHistory
            }
            onUpdateBalance={onUpdateBalance}
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
          <ButtonRow noMargin={true}>
            {isReduxSelectedAssetBuySupported &&
              <BridgeToAuroraButton
                onClick={onSelectBuy}
              >
                {getLocale('braveWalletBuy')}
              </BridgeToAuroraButton>
            }
            {isSelectedAssetDepositSupported &&
              <BridgeToAuroraButton
                onClick={onSelectDeposit}
              >
                {getLocale('braveWalletAccountsDeposit')}
              </BridgeToAuroraButton>
            }
            {isSelectedAssetBridgeSupported &&
              <BridgeToAuroraButton
                onClick={onBridgeToAuroraButton}
              >
                {getLocale('braveWalletBridgeToAuroraButton')}
              </BridgeToAuroraButton>
            }
          </ButtonRow>
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
            formattedFiatBalance={
              fullAssetFiatBalance.formatAsFiat(defaultCurrencies.fiat)
            }
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

        {!nftMetadataError &&
          <NftMultimedia
            onLoad={onNftDetailsLoad}
            visible={selectedAsset?.isErc721 || selectedAsset?.isNft}
            ref={nftDetailsRef}
            sandbox="allow-scripts allow-popups allow-same-origin"
            allow="clipboard-write"
            src='chrome-untrusted://nft-display'
            allowFullScreen
          />
        }

        {isNftAsset && selectedAsset &&
          <NftDetails
            selectedAsset={selectedAsset}
            nftMetadata={nftMetadata}
            nftMetadataError={nftMetadataError}
            tokenNetwork={selectedAssetsNetwork}
            nftPinningStatus={currentNftPinningStatus}
            imageIpfsUrl={ipfsImageUrl}
          />
        }

        {showNftModal && nftMetadata?.imageURL &&
          <NftModal
            nftImageUrl={nftMetadata.imageURL}
            onClose={onCloseNftModal}
          />
        }

        {!isShowingMarketData &&
          <AccountsAndTransactionsList
            formattedFullAssetBalance={formattedFullAssetBalance}
            fullAssetFiatBalance={fullAssetFiatBalance}
            selectedAsset={selectedAsset}
            selectedAssetTransactions={selectedAssetTransactions}
            onClickAddAccount={onClickAddAccount}
          />
        }

        {isShowingMarketData && selectedCoinMarket &&
          <CoinStats
            marketCapRank={selectedCoinMarket.marketCapRank}
            volume={selectedCoinMarket.totalVolume}
            marketCap={selectedCoinMarket.marketCap}
          />
        }
      </ScrollableColumn>
    </StyledWrapper>
  )
}

export default PortfolioAsset
