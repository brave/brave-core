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
  BraveWallet,
  WalletRoutes,
  TokenPriceHistory
} from '../../../../constants/types'

// Utils
import Amount from '../../../../utils/amount'
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
import { makeDepositFundsRoute } from '../../../../utils/routes-utils'
import {
  getIsRewardsToken,
  getRewardsBATToken
} from '../../../../utils/rewards_utils'

// actions
import { WalletPageActions } from '../../../../page/actions'

// selectors
import { WalletSelectors } from '../../../../common/selectors'
import { PageSelectors } from '../../../../page/selectors'

// Components
import { LineChart } from '../../line-chart/index'
import {
  LineChartControls
} from '../../line-chart/line-chart-controls/line-chart-controls'
import AccountsAndTransactionsList from './components/accounts-and-transctions-list'
import { BridgeToAuroraModal } from '../../popup-modals/bridge-to-aurora-modal/bridge-to-aurora-modal'

// Hooks
import {
  useScopedBalanceUpdater
} from '../../../../common/hooks/use-scoped-balance-updater'
import {
  useIsBuySupported //
} from '../../../../common/hooks/use-multi-chain-buy-assets'
import {
  useUnsafePageSelector,
  useUnsafeWalletSelector
} from '../../../../common/hooks/use-safe-selector'
import {
  useGetNetworkQuery,
  useGetTransactionsQuery,
  useGetTokenSpotPricesQuery,
  useGetPriceHistoryQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetRewardsEnabledQuery,
  useGetExternalRewardsWalletQuery
} from '../../../../common/slices/api.slice'
import {
  useAccountsQuery,
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

const rainbowbridgeLink = 'https://rainbowbridge.app'
const bridgeToAuroraDontShowAgainKey = 'bridgeToAuroraDontShowAgain'

interface Props {
  isShowingMarketData?: boolean
}

const emptyPriceList: TokenPriceHistory[] = []

export const PortfolioAsset = (props: Props) => {
  const { isShowingMarketData } = props

  // state
  const [showBridgeToAuroraModal, setShowBridgeToAuroraModal] = React.useState<boolean>(false)
  const [dontShowAuroraWarning, setDontShowAuroraWarning] = React.useState<boolean>(false)
  const [showTokenDetailsModal, setShowTokenDetailsModal] = React.useState<boolean>(false)
  const [showHideTokenModel, setShowHideTokenModal] = React.useState<boolean>(false)
  const [selectedTimeline, setSelectedTimeline] = React.useState<number>(
    BraveWallet.AssetPriceTimeframe.OneDay
  )

  // routing
  const history = useHistory()
  const { chainIdOrMarketSymbol, contractOrSymbol } = useParams<{
    chainIdOrMarketSymbol?: string
    contractOrSymbol?: string
  }>()

  // redux
  const dispatch = useDispatch()

  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)
  const userVisibleTokensInfo = useUnsafeWalletSelector(WalletSelectors.userVisibleTokensInfo)
  const coinMarketData = useUnsafeWalletSelector(WalletSelectors.coinMarketData)
  const selectedCoinMarket = useUnsafePageSelector(PageSelectors.selectedCoinMarket)

  // Queries
  const { data: isRewardsEnabled } = useGetRewardsEnabledQuery()
  const { data: externalRewardsInfo } = useGetExternalRewardsWalletQuery()

  // Memos
  const userTokensInfo = React.useMemo(() => {
    const rewardsToken =
      getRewardsBATToken(externalRewardsInfo?.provider ?? undefined)
    return isRewardsEnabled &&
      rewardsToken
      ? [rewardsToken, ...userVisibleTokensInfo]
      : userVisibleTokensInfo
  }, [
    isRewardsEnabled,
    userVisibleTokensInfo
  ])

  // params
  const selectedAssetFromParams = React.useMemo(() => {
    if (!chainIdOrMarketSymbol) {
      return undefined
    }

    if (isShowingMarketData) {
      const marketSymbolLower = chainIdOrMarketSymbol.toLowerCase()
      const coinMarket = coinMarketData.find(
        (token) =>
          token.symbol.toLowerCase() === marketSymbolLower
      )
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
    const contractOrSymbolLower = contractOrSymbol.toLowerCase()
    const userToken = userTokensInfo.find(
      (token) =>
        (token.contractAddress.toLowerCase() === contractOrSymbolLower &&
          token.chainId === chainIdOrMarketSymbol) ||
        (token.symbol.toLowerCase() === contractOrSymbolLower &&
          token.chainId === chainIdOrMarketSymbol &&
          token.contractAddress === '')
    )
    return userToken
  }, [
    userTokensInfo,
    selectedTimeline,
    chainIdOrMarketSymbol,
    contractOrSymbol,
    isShowingMarketData
  ])

  const isRewardsToken = getIsRewardsToken(selectedAssetFromParams)

  // queries
  const { accounts } = useAccountsQuery()

  const { data: combinedTokensList } = useGetCombinedTokensListQuery()

  const { data: defaultFiat } = useGetDefaultFiatCurrencyQuery()

  const { data: selectedAssetsNetwork } = useGetNetworkQuery(
    selectedAssetFromParams ?? skipToken
  )

  const { data: transactionsByNetwork = [] } = useGetTransactionsQuery(
    selectedAssetFromParams
      ? {
        accountId: null,
        chainId: selectedAssetFromParams.chainId,
        coinType: selectedAssetFromParams.coin
      }
      : skipToken
  )

  const candidateAccounts = React.useMemo(() => {
    if (!selectedAssetFromParams) {
      return []
    }

    return accounts.filter((account) =>
      account.accountId.coin === selectedAssetFromParams.coin)
  }, [accounts, selectedAssetFromParams])

  const {
    data: tokenBalancesRegistry,
  } = useScopedBalanceUpdater(
    selectedAssetFromParams && candidateAccounts && selectedAssetsNetwork
      ? {
          network: selectedAssetsNetwork,
          accounts: candidateAccounts,
          tokens: [selectedAssetFromParams]
        }
      : skipToken
  )

  const {
    data: selectedAssetPriceHistory,
    isLoading: isFetchingPortfolioPriceHistory
  } = useGetPriceHistoryQuery(
    selectedAssetFromParams && defaultFiat
      ? {
          tokenParam: getPriceIdForToken(selectedAssetFromParams),
          timeFrame: selectedTimeline,
          vsAsset: defaultFiat
        }
      : skipToken
    )

  // custom hooks
  const isAssetBuySupported =
    useIsBuySupported(selectedAssetFromParams) &&
    !isRewardsToken

  // memos
  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useMemo(() => {
    const amounts = candidateAccounts.map((account: BraveWallet.AccountInfo) =>
      getBalance(
        account.accountId,
        selectedAssetFromParams,
        tokenBalancesRegistry
      )
    )

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
  }, [candidateAccounts, selectedAssetFromParams, tokenBalancesRegistry])

  // memos / computed

  const tokenPriceIds = React.useMemo(() =>
    selectedAssetFromParams && new Amount(fullAssetBalance).gt(0)
      ? [getPriceIdForToken(selectedAssetFromParams)]
      : [],
    [fullAssetBalance, selectedAssetFromParams]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiat
      ? { ids: tokenPriceIds, toCurrency: defaultFiat }
      : skipToken,
    querySubscriptionOptions60s
  )

  const isSelectedAssetBridgeSupported = React.useMemo(() => {
    if (!selectedAssetFromParams) return false
    const isBridgeAddress = auroraSupportedContractAddresses.includes(selectedAssetFromParams.contractAddress.toLowerCase())
    const isNativeAsset = selectedAssetFromParams.contractAddress === ''

    return (isBridgeAddress || isNativeAsset) && selectedAssetFromParams.chainId === BraveWallet.MAINNET_CHAIN_ID
  }, [selectedAssetFromParams])

  const selectedAssetTransactions = React.useMemo(() => {
    if (selectedAssetFromParams) {
      const filteredTransactions = transactionsByNetwork.filter((tx) => {
        const token = findTransactionToken(tx, [selectedAssetFromParams])

        const { sellToken, buyToken } = getETHSwapTransactionBuyAndSellTokens({
          nativeAsset: makeNetworkAsset(selectedAssetsNetwork),
          tokensList: [selectedAssetFromParams],
          tx
        })
        const selectedAssetIdKey = getAssetIdKey(selectedAssetFromParams)
        return (
          (token && selectedAssetIdKey === getAssetIdKey(token)) ||
          (sellToken && selectedAssetIdKey === getAssetIdKey(sellToken)) ||
          (buyToken && selectedAssetIdKey === getAssetIdKey(buyToken))
        )
      })
      return sortTransactionByDate(filteredTransactions, 'descending')
    }
    return []
  }, [
    selectedAssetFromParams,
    transactionsByNetwork,
    selectedAssetsNetwork
  ])

  const fullAssetFiatBalance = React.useMemo(() =>
    selectedAssetFromParams && fullAssetBalance
      ? computeFiatAmount({
          spotPriceRegistry,
          value: fullAssetBalance,
          token: selectedAssetFromParams
        })
      : Amount.empty(),
    [fullAssetBalance, selectedAssetFromParams, spotPriceRegistry]
  )

  const formattedFullAssetBalance = React.useMemo(() =>
    selectedAssetFromParams && fullAssetBalance
      ? new Amount(fullAssetBalance)
        .divideByDecimals(selectedAssetFromParams.decimals)
        .formatAsAsset(6, selectedAssetFromParams.symbol)
      : '',
    [selectedAssetFromParams, fullAssetBalance]
  )

  const formattedAssetBalance = React.useMemo(() =>
    selectedAssetFromParams && fullAssetBalance
      ? new Amount(fullAssetBalance)
        .divideByDecimals(selectedAssetFromParams.decimals)
        .formatAsAsset(8)
      : '',
    [selectedAssetFromParams, fullAssetBalance]
  )

  const isSelectedAssetDepositSupported = React.useMemo(() => {
    if (!selectedAssetFromParams || isRewardsToken) {
      return false
    }

    return combinedTokensList.some(
      (token) =>
        token.symbol.toLowerCase() ===
        selectedAssetFromParams.symbol.toLowerCase()
    )
  }, [
    combinedTokensList,
    selectedAssetFromParams?.symbol,
    isRewardsToken
  ])

  const goBack = React.useCallback(() => {
    dispatch(WalletPageActions.selectCoinMarket(undefined))
    dispatch(WalletPageActions.updateNFTMetadata(undefined))
    dispatch(WalletPageActions.updateNftMetadataError(undefined))
    if (isShowingMarketData) {
      history.push(WalletRoutes.Market)
      return
    }
    history.push(WalletRoutes.PortfolioAssets)
  }, [
    isShowingMarketData,
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
    if (!selectedAssetFromParams) return
    dispatch(
      WalletActions.setUserAssetVisible({
        token: selectedAssetFromParams,
        isVisible: false
      })
    )
    dispatch(WalletActions.refreshBalancesAndPriceHistory())
    dispatch(WalletPageActions.selectAsset({
      asset: undefined,
      timeFrame: BraveWallet.AssetPriceTimeframe.OneDay
    }))
    if (showHideTokenModel) setShowHideTokenModal(false)
    if (showTokenDetailsModal) setShowTokenDetailsModal(false)
    history.push(WalletRoutes.PortfolioAssets)
  }, [selectedAssetFromParams, showTokenDetailsModal])

  const onSelectBuy = React.useCallback(() => {
    history.push(
      `${WalletRoutes.FundWalletPageStart}/${selectedAssetFromParams?.symbol}`
    )
  }, [selectedAssetFromParams?.symbol])

  const onSelectDeposit = React.useCallback(() => {
    history.push(makeDepositFundsRoute(selectedAssetFromParams?.symbol))
  }, [selectedAssetFromParams?.symbol])

  React.useEffect(() => {
    setDontShowAuroraWarning(JSON.parse(localStorage.getItem(bridgeToAuroraDontShowAgainKey) || 'false'))
  }, [])

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
      noCardPadding={true}
      hideDivider={true}
      cardHeader={
        <AssetDetailsHeader
          selectedAsset={selectedAssetFromParams}
          isShowingMarketData={isShowingMarketData}
          onBack={goBack}
          onClickTokenDetails={
            () => setShowTokenDetailsModal(true)
          }
          onClickHideToken={
            () => setShowHideTokenModal(true)
          }
        />
      }
    >
      <StyledWrapper>
        <Row
          margin='20px 0px 8px 0px'
        >
          <LineChartControls
            onSelectTimeline={setSelectedTimeline}
            selectedTimeline={selectedTimeline}
          />
        </Row>

        <LineChart
          priceData={
            selectedAssetFromParams && selectedAssetPriceHistory
              ? selectedAssetPriceHistory
              : emptyPriceList
          }
          isLoading={
            !selectedAssetFromParams || isFetchingPortfolioPriceHistory
          }
          isDisabled={
            !selectedAssetFromParams || isFetchingPortfolioPriceHistory
          }
        />
        <Row
          padding='0px 20px'
        >
          <ButtonRow>
            {isAssetBuySupported &&
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

        {showBridgeToAuroraModal &&
          <BridgeToAuroraModal
            dontShowWarningAgain={dontShowAuroraWarning}
            onClose={onCloseAuroraModal}
            onOpenRainbowAppClick={onOpenRainbowAppClick}
            onDontShowAgain={onDontShowAgain}
          />
        }

        {showTokenDetailsModal &&
          selectedAssetFromParams &&
          selectedAssetsNetwork &&
          <TokenDetailsModal
            onClose={onCloseTokenDetailsModal}
            selectedAsset={selectedAssetFromParams}
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
          selectedAssetFromParams &&
          selectedAssetsNetwork &&
          <HideTokenModal
            selectedAsset={selectedAssetFromParams}
            selectedAssetNetwork={selectedAssetsNetwork}
            onClose={onCloseHideTokenModal}
            onHideAsset={onHideAsset}
          />
        }

        {!isShowingMarketData &&
          <Column
            padding='0px 24px 24px 24px'
            fullWidth={true}
          >
            <AccountsAndTransactionsList
              formattedFullAssetBalance={formattedFullAssetBalance}
              fullAssetFiatBalance={fullAssetFiatBalance}
              selectedAsset={selectedAssetFromParams}
              selectedAssetTransactions={selectedAssetTransactions}
              tokenBalancesRegistry={tokenBalancesRegistry}
              accounts={candidateAccounts}
              spotPriceRegistry={spotPriceRegistry}
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
