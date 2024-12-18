// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { Redirect, useHistory, useParams } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import {
  BraveWallet,
  WalletRoutes,
  TokenPriceHistory,
  LineChartIframeData
} from '../../../../constants/types'

// constants
import { emptyRewardsInfo } from '../../../../common/async/base-query-cache'
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../common/constants/local-storage-keys'

// Utils
import Amount from '../../../../utils/amount'
import {
  findTransactionToken,
  getETHSwapTransactionBuyAndSellTokens,
  sortTransactionByDate
} from '../../../../utils/tx-utils'
import { getBalance } from '../../../../utils/balance-utils'
import {
  computeFiatAmount,
  getPriceIdForToken
} from '../../../../utils/pricing-utils'
import { networkSupportsAccount } from '../../../../utils/network-utils'
import { getAssetIdKey } from '../../../../utils/asset-utils'
import { getLocale } from '../../../../../common/locale'
import { makeNetworkAsset } from '../../../../options/asset-options'
import { isRewardsAssetId } from '../../../../utils/rewards_utils'
import {
  makeAndroidFundWalletRoute,
  makeDepositFundsRoute,
  makeFundWalletRoute
} from '../../../../utils/routes-utils'
import {
  getStoredPortfolioTimeframe //
} from '../../../../utils/local-storage-utils'

// actions
import { WalletPageActions } from '../../../../page/actions'

// Components
import {
  LineChartControls //
} from '../../line-chart/line-chart-controls/line-chart-controls'
import {
  AccountsAndTransactionsList //
} from './components/accounts-and-transctions-list'
import {
  EditTokenModal //
} from '../../popup-modals/edit_token_modal/edit_token_modal'

// Hooks
import {
  useScopedBalanceUpdater //
} from '../../../../common/hooks/use-scoped-balance-updater'
import { useFindBuySupportedToken } from '../../../../common/hooks/use-multi-chain-buy-assets'
import {
  useGetNetworkQuery,
  useGetTransactionsQuery,
  useGetTokenSpotPricesQuery,
  useGetPriceHistoryQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetRewardsInfoQuery,
  useGetUserTokensRegistryQuery,
  useUpdateUserAssetVisibleMutation
} from '../../../../common/slices/api.slice'
import { useAccountsQuery } from '../../../../common/slices/api.slice.extra'
import {
  querySubscriptionOptions60s //
} from '../../../../common/slices/constants'
import {
  useSyncedLocalStorage //
} from '../../../../common/hooks/use_local_storage'

// Styled Components
import { StyledWrapper, ButtonRow } from './style'
import { Row, Column, LeoSquaredButton } from '../../../shared/style'
import {
  TokenDetailsModal //
} from './components/token-details-modal/token-details-modal'
import {
  WalletActions //
} from '../../../../common/actions'
import { HideTokenModal } from './components/hide-token-modal/hide-token-modal'
import {
  WalletPageWrapper //
} from '../../wallet-page-wrapper/wallet-page-wrapper'
import { AssetDetailsHeader } from '../../card-headers/asset-details-header'

const emptyPriceList: TokenPriceHistory[] = []

export const PortfolioFungibleAsset = () => {
  // state
  const [showTokenDetailsModal, setShowTokenDetailsModal] =
    React.useState<boolean>(false)
  const [showHideTokenModel, setShowHideTokenModal] =
    React.useState<boolean>(false)
  const [selectedTimeline, setSelectedTimeline] = React.useState<number>(
    getStoredPortfolioTimeframe
  )
  const [showEditTokenModal, setShowEditTokenModal] =
    React.useState<boolean>(false)

  // routing
  const history = useHistory()
  const { assetId } = useParams<{
    assetId?: string
  }>()
  const isRewardsToken = assetId ? isRewardsAssetId(assetId) : false

  // redux
  const dispatch = useDispatch()

  // Local-Storage
  const [hidePortfolioBalances] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
    false
  )

  // Queries
  const { data: userTokensRegistry, isLoading: isLoadingTokens } =
    useGetUserTokensRegistryQuery()
  const { data: defaultFiat = 'USD' } = useGetDefaultFiatCurrencyQuery()
  const {
    data: { rewardsToken } = emptyRewardsInfo,
    isLoading: isLoadingRewards
  } = useGetRewardsInfoQuery(isRewardsToken ? undefined : skipToken)

  // params
  const selectedAssetFromParams = React.useMemo(() => {
    if (isRewardsToken) {
      return rewardsToken
    }
    return assetId ? userTokensRegistry?.entities[assetId] : undefined
  }, [isRewardsToken, rewardsToken, assetId, userTokensRegistry])

  // mutations
  const [updateUserAssetVisible] = useUpdateUserAssetVisibleMutation()

  // queries
  const { accounts } = useAccountsQuery()

  const { data: selectedAssetsNetwork } = useGetNetworkQuery(
    selectedAssetFromParams ?? skipToken
  )

  const { data: transactionsByNetwork = [] } = useGetTransactionsQuery(
    selectedAssetFromParams && !isRewardsToken
      ? {
          accountId: null,
          chainId: selectedAssetFromParams.chainId,
          coinType: selectedAssetFromParams.coin
        }
      : skipToken
  )

  const candidateAccounts = React.useMemo(() => {
    if (!selectedAssetsNetwork) {
      return []
    }

    return accounts.filter((account) =>
      networkSupportsAccount(selectedAssetsNetwork, account.accountId)
    )
  }, [selectedAssetsNetwork, accounts])

  const { data: tokenBalancesRegistry, isLoading: isLoadingBalances } =
    useScopedBalanceUpdater(
      selectedAssetFromParams && candidateAccounts && selectedAssetsNetwork
        ? {
            network: selectedAssetsNetwork,
            accounts: candidateAccounts,
            tokens: [selectedAssetFromParams]
          }
        : skipToken
    )

  const tokenPriceIds = React.useMemo(
    () =>
      selectedAssetFromParams
        ? [getPriceIdForToken(selectedAssetFromParams)]
        : [],
    [selectedAssetFromParams]
  )

  const {
    data: selectedAssetPriceHistory,
    isFetching: isFetchingPortfolioPriceHistory
  } = useGetPriceHistoryQuery(
    selectedAssetFromParams && defaultFiat
      ? {
          tokenParam: tokenPriceIds[0],
          timeFrame: selectedTimeline,
          vsAsset: defaultFiat
        }
      : skipToken
  )

  // custom hooks
  const { foundAndroidBuyToken, foundMeldBuyToken } = useFindBuySupportedToken(
    selectedAssetFromParams
  )

  // memos
  /**
   * This will scrape all the user's accounts and combine the asset balances for
   * a single asset
   */
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
      return a !== '' && b !== '' ? new Amount(a).plus(b).format() : ''
    })
  }, [candidateAccounts, selectedAssetFromParams, tokenBalancesRegistry])

  // memos / computed
  const isLoadingGraphData =
    !selectedAssetFromParams || isFetchingPortfolioPriceHistory

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiat
      ? { ids: tokenPriceIds, toCurrency: defaultFiat }
      : skipToken,
    querySubscriptionOptions60s
  )

  const selectedAssetTransactions = React.useMemo(() => {
    const nativeAsset = makeNetworkAsset(selectedAssetsNetwork)

    if (selectedAssetFromParams) {
      const filteredTransactions = transactionsByNetwork.filter((tx) => {
        const token = findTransactionToken(tx, [selectedAssetFromParams])

        const selectedAssetIdKey = getAssetIdKey(selectedAssetFromParams)
        const tokenId = token ? getAssetIdKey(token) : undefined

        if (tx.txType === BraveWallet.TransactionType.ETHSwap) {
          const { sellToken, buyToken } = getETHSwapTransactionBuyAndSellTokens(
            {
              nativeAsset,
              tokensList: [selectedAssetFromParams],
              tx
            }
          )
          const buyTokenId = buyToken ? getAssetIdKey(buyToken) : undefined
          const sellTokenId = sellToken ? getAssetIdKey(sellToken) : undefined
          return (
            selectedAssetIdKey === tokenId ||
            selectedAssetIdKey === buyTokenId ||
            selectedAssetIdKey === sellTokenId
          )
        }

        return selectedAssetIdKey === tokenId
      })
      return sortTransactionByDate(filteredTransactions, 'descending')
    }
    return []
  }, [selectedAssetFromParams, transactionsByNetwork, selectedAssetsNetwork])

  const fullAssetFiatBalance = React.useMemo(
    () =>
      selectedAssetFromParams && fullAssetBalance
        ? computeFiatAmount({
            spotPriceRegistry,
            value: fullAssetBalance,
            token: selectedAssetFromParams
          })
        : Amount.empty(),
    [fullAssetBalance, selectedAssetFromParams, spotPriceRegistry]
  )

  const formattedFullAssetBalance = React.useMemo(
    () =>
      selectedAssetFromParams && fullAssetBalance
        ? new Amount(fullAssetBalance)
            .divideByDecimals(selectedAssetFromParams.decimals)
            .formatAsAsset(6, selectedAssetFromParams.symbol)
        : '',
    [selectedAssetFromParams, fullAssetBalance]
  )

  const formattedAssetBalance = React.useMemo(
    () =>
      selectedAssetFromParams && fullAssetBalance
        ? new Amount(fullAssetBalance)
            .divideByDecimals(selectedAssetFromParams.decimals)
            .formatAsAsset(8)
        : '',
    [selectedAssetFromParams, fullAssetBalance]
  )

  const isSelectedAssetDepositSupported =
    !isRewardsToken && Boolean(selectedAssetFromParams)

  const goBack = React.useCallback(() => {
    dispatch(WalletPageActions.updateNFTMetadata(undefined))
    dispatch(WalletPageActions.updateNftMetadataError(undefined))
    history.push(WalletRoutes.PortfolioAssets)
  }, [dispatch, history])

  const onCloseTokenDetailsModal = React.useCallback(
    () => setShowTokenDetailsModal(false),
    []
  )

  const onCloseHideTokenModal = React.useCallback(
    () => setShowHideTokenModal(false),
    []
  )

  const onHideAsset = React.useCallback(async () => {
    if (!selectedAssetFromParams) return
    await updateUserAssetVisible({
      token: selectedAssetFromParams,
      isVisible: false
    }).unwrap()
    dispatch(WalletActions.refreshBalancesAndPriceHistory())
    if (showHideTokenModel) setShowHideTokenModal(false)
    if (showTokenDetailsModal) setShowTokenDetailsModal(false)
    history.push(WalletRoutes.PortfolioAssets)
  }, [
    dispatch,
    history,
    selectedAssetFromParams,
    showHideTokenModel,
    showTokenDetailsModal,
    updateUserAssetVisible
  ])

  const onSelectBuy = React.useCallback(() => {
    if (foundAndroidBuyToken && selectedAssetFromParams) {
      history.push(
        makeAndroidFundWalletRoute(getAssetIdKey(selectedAssetFromParams))
      )
      return
    }
    if (foundMeldBuyToken) {
      history.push(makeFundWalletRoute(foundMeldBuyToken))
    }
  }, [
    history,
    foundAndroidBuyToken,
    foundMeldBuyToken,
    selectedAssetFromParams
  ])

  const onSelectDeposit = React.useCallback(() => {
    if (selectedAssetFromParams) {
      history.push(
        makeDepositFundsRoute(getAssetIdKey(selectedAssetFromParams))
      )
    }
  }, [history, selectedAssetFromParams])

  // asset not found
  if (!selectedAssetFromParams && !isLoadingRewards && !isLoadingTokens) {
    return <Redirect to={WalletRoutes.PortfolioAssets} />
  }

  const priceData =
    selectedAssetFromParams && selectedAssetPriceHistory
      ? selectedAssetPriceHistory
      : emptyPriceList

  const iframeData: LineChartIframeData = {
    priceData,
    hidePortfolioBalances,
    defaultFiatCurrency: defaultFiat || 'USD'
  }

  const encodedPriceData = encodeURIComponent(JSON.stringify(iframeData))

  // render
  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      noCardPadding={true}
      useCardInPanel={true}
      cardHeader={
        <AssetDetailsHeader
          selectedTimeline={selectedTimeline}
          selectedAsset={selectedAssetFromParams}
          isShowingMarketData={false}
          onBack={goBack}
          onClickTokenDetails={() => setShowTokenDetailsModal(true)}
          onClickHideToken={() => setShowHideTokenModal(true)}
          onClickEditToken={
            selectedAssetFromParams?.contractAddress !== ''
              ? () => setShowEditTokenModal(true)
              : undefined
          }
        />
      }
    >
      <StyledWrapper>
        <Row margin='20px 0px 8px 0px'>
          <LineChartControls
            onSelectTimeline={setSelectedTimeline}
            selectedTimeline={selectedTimeline}
          />
        </Row>

        <iframe
          width={'100%'}
          height={'130px'}
          frameBorder={0}
          src={`chrome-untrusted://line-chart-display${
            isLoadingGraphData ? '' : `?${encodedPriceData}`
          }`}
          sandbox='allow-scripts allow-same-origin'
        />
        <Row padding='0px 20px'>
          <ButtonRow>
            {(foundMeldBuyToken || foundAndroidBuyToken) && !isRewardsToken && (
              <div>
                <LeoSquaredButton onClick={onSelectBuy}>
                  {getLocale('braveWalletBuy')}
                </LeoSquaredButton>
              </div>
            )}
            {isSelectedAssetDepositSupported && (
              <div>
                <LeoSquaredButton onClick={onSelectDeposit}>
                  {getLocale('braveWalletAccountsDeposit')}
                </LeoSquaredButton>
              </div>
            )}
          </ButtonRow>
        </Row>

        {showTokenDetailsModal &&
          selectedAssetFromParams &&
          selectedAssetsNetwork && (
            <TokenDetailsModal
              onClose={onCloseTokenDetailsModal}
              selectedAsset={selectedAssetFromParams}
              selectedAssetNetwork={selectedAssetsNetwork}
              assetBalance={formattedAssetBalance}
              formattedFiatBalance={fullAssetFiatBalance.formatAsFiat(
                defaultFiat
              )}
              onShowHideTokenModal={() => setShowHideTokenModal(true)}
            />
          )}

        {showHideTokenModel &&
          selectedAssetFromParams &&
          selectedAssetsNetwork && (
            <HideTokenModal
              selectedAsset={selectedAssetFromParams}
              onClose={onCloseHideTokenModal}
              onHideAsset={onHideAsset}
            />
          )}

        {showEditTokenModal && selectedAssetFromParams && (
          <EditTokenModal
            onClose={() => setShowEditTokenModal(false)}
            token={selectedAssetFromParams}
          />
        )}

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
            isLoadingBalances={isLoadingBalances}
            accounts={candidateAccounts}
            spotPriceRegistry={spotPriceRegistry}
          />
        </Column>
      </StyledWrapper>
    </WalletPageWrapper>
  )
}

export default PortfolioFungibleAsset
