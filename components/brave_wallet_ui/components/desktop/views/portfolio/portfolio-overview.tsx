// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useDispatch } from 'react-redux'
import { useHistory } from 'react-router'
import {
  Route,
  Switch,
  Redirect
} from 'react-router-dom'

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
  WalletRoutes
} from '../../../../constants/types'
import {
  LOCAL_STORAGE_KEYS
} from '../../../../common/constants/local-storage-keys'

// actions
import { WalletActions } from '../../../../common/actions'
import { WalletPageActions } from '../../../../page/actions'

// Utils
import Amount from '../../../../utils/amount'
import { getBalance } from '../../../../utils/balance-utils'
import {
  computeFiatAmount,
  getTokenPriceAmountFromRegistry
} from '../../../../utils/pricing-utils'
import { getAssetIdKey } from '../../../../utils/asset-utils'
import { formatAsDouble } from '../../../../utils/string-utils'
import { getPriceIdForToken } from '../../../../utils/api-utils'
import {
  networkEntityAdapter
} from '../../../../common/slices/entities/network.entity'

// Options
import { PortfolioNavOptions } from '../../../../options/nav-options'
import {
  AccountsGroupByOption
} from '../../../../options/group-assets-by-options'

// Components
import { LoadingSkeleton } from '../../../shared'
import {
  SegmentedControl
} from '../../../shared/segmented-control/segmented-control'
import { PortfolioAssetItem } from '../../'
import { TokenLists } from './components/token-lists/token-list'
import {
  PortfolioOverviewChart //
} from './components/portfolio-overview-chart/portfolio-overview-chart'
import ColumnReveal from '../../../shared/animated-reveals/column-reveal'
import { NftView } from '../nfts/nft-view'
import {
  BuySendSwapDepositNav
} from './components/buy-send-swap-deposit-nav/buy-send-swap-deposit-nav'
import {
  LineChartControls
} from '../../line-chart/line-chart-controls/line-chart-controls'
import {
  PortfolioFiltersModal
} from '../../popup-modals/filter-modals/portfolio-filters-modal'

// Styled Components
import {
  BalanceText,
  PercentBubble,
  FiatChange,
  SelectTimelineWrapper,
  ControlsRow,
  BalanceAndButtonsWrapper,
  BalanceAndChangeWrapper
} from './style'
import {
  Column,
  Row,
  HorizontalSpace
} from '../../../shared/style'
import {
  useGetVisibleNetworksQuery,
  useGetTokenSpotPricesQuery
} from '../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../../common/slices/constants'

interface Props {
  onToggleShowIpfsBanner: () => void
}

export const PortfolioOverview = ({ onToggleShowIpfsBanner }: Props) => {
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
  const selectedTimeline =
    useSafePageSelector(PageSelectors.selectedTimeline)
  const nftMetadata =
    useUnsafePageSelector(PageSelectors.nftMetadata)
  const hidePortfolioGraph =
    useSafeWalletSelector(WalletSelectors.hidePortfolioGraph)
  const hidePortfolioBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioBalances)
  const portfolioPriceHistory =
    useUnsafeWalletSelector(WalletSelectors.portfolioPriceHistory)
  const hidePortfolioNFTsTab =
    useSafeWalletSelector(WalletSelectors.hidePortfolioNFTsTab)
  const filteredOutPortfolioNetworkKeys =
    useUnsafeWalletSelector(WalletSelectors.filteredOutPortfolioNetworkKeys)
  const filteredOutPortfolioAccountAddresses =
    useUnsafeWalletSelector(
      WalletSelectors.filteredOutPortfolioAccountAddresses
    )
  const hidePortfolioSmallBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioSmallBalances)
  const selectedGroupAssetsByItem =
    useSafeWalletSelector(WalletSelectors.selectedGroupAssetsByItem)

  // queries
  const { data: networks } = useGetVisibleNetworksQuery()

  // State
  const [showPortfolioSettings, setShowPortfolioSettings] =
    React.useState<boolean>(false)

  const usersFilteredAccounts = React.useMemo(() => {
    return accounts
      .filter(
        (account) =>
          !filteredOutPortfolioAccountAddresses
            .includes(account.address)
      )
  }, [accounts, filteredOutPortfolioAccountAddresses])

  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const amounts = usersFilteredAccounts
      .filter((account) => account.accountId.coin === asset.coin)
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
  }, [usersFilteredAccounts, getBalance])

  // memos / computed

  // Filters the user's tokens based on the users
  // filteredOutPortfolioNetworkKeys pref.
  const visibleTokensForFilteredChains = React.useMemo(() => {
    return userVisibleTokensInfo
      .filter(
        (token) =>
          !filteredOutPortfolioNetworkKeys
            .includes(
              networkEntityAdapter
                .selectId(
                  {
                    chainId: token.chainId,
                    coin: token.coin
                  }
                ).toString()
            )
      )
  }, [
    filteredOutPortfolioNetworkKeys,
    userVisibleTokensInfo
  ])

  // This looks at the users asset list and returns the full balance for each asset
  const userAssetList: UserAssetInfoType[] = React.useMemo(() => {
    return visibleTokensForFilteredChains.map((asset) => ({
      asset: asset,
      assetBalance: fullAssetBalance(asset)
    }))
  }, [
    visibleTokensForFilteredChains,
    fullAssetBalance,
  ])

  const visibleAssetOptions = React.useMemo((): UserAssetInfoType[] => {
    return userAssetList.filter(({ asset }) => asset.visible && !(asset.isErc721 || asset.isNft))
  }, [userAssetList])

  const allNetworksAreHidden = React.useMemo(() => {
    return networks
      .every(
        (network) =>
          filteredOutPortfolioNetworkKeys
            .includes(
              networkEntityAdapter
                .selectId(network)
                .toString()
            )
      )
  }, [networks, filteredOutPortfolioNetworkKeys])

  const tokenPriceIds = React.useMemo(() =>
    visibleAssetOptions
      .filter(({ assetBalance }) => new Amount(assetBalance).gt(0))
      .filter(({ asset }) =>
        !asset.isErc721 && !asset.isErc1155 && !asset.isNft)
      .map(({ asset }) => getPriceIdForToken(asset)),
    [visibleAssetOptions]
  )

  const {
    data: spotPriceRegistry,
    isLoading: isLoadingSpotPrices,
    isFetching: isFetchingSpotPrices
  } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length ? { ids: tokenPriceIds } : skipToken,
    querySubscriptionOptions60s
  )

  // This will scrape all of the user's accounts and combine the fiat value for every asset
  const fullPortfolioFiatBalance = React.useMemo((): Amount => {
    if (allNetworksAreHidden) {
      return Amount.zero()
    }

    if (visibleAssetOptions.length === 0) {
      return Amount.empty()
    }

    if (isLoadingSpotPrices || isFetchingSpotPrices) {
      return Amount.empty()
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
    return grandTotal
  },
    [
      visibleAssetOptions,
      spotPriceRegistry,
      isLoadingSpotPrices,
      isFetchingSpotPrices,
      allNetworksAreHidden
    ])

  const formattedFullPortfolioFiatBalance = React.useMemo(() => {
    return !fullPortfolioFiatBalance.isUndefined()
      ? fullPortfolioFiatBalance
        .formatAsFiat(defaultCurrencies.fiat)
      : ''
  }, [
    fullPortfolioFiatBalance,
    defaultCurrencies.fiat
  ])

  const isZeroBalance = React.useMemo((): boolean => {
    // In some cases we need to check if the balance is zero
    return parseFloat(formatAsDouble(fullPortfolioFiatBalance.format())) === 0
  }, [fullPortfolioFiatBalance])

  const percentageChange = React.useMemo(() => {
    if (portfolioPriceHistory.length !== 0 && !isZeroBalance) {
      const oldestValue =
        new Amount(portfolioPriceHistory[0].close)
      const difference =
        fullPortfolioFiatBalance.minus(oldestValue)
      return `${difference.div(oldestValue).times(100).format(2)}`
    }
    return '0.00'
  }, [
    portfolioPriceHistory,
    fullPortfolioFiatBalance,
    isZeroBalance
  ])

  const fiatValueChange = React.useMemo(() => {
    if (portfolioPriceHistory.length !== 0 && !isZeroBalance) {
      const oldestValue =
        new Amount(portfolioPriceHistory[0].close)
      return fullPortfolioFiatBalance
        .minus(oldestValue)
        .formatAsFiat(defaultCurrencies.fiat)
    }
    return new Amount(0).formatAsFiat(defaultCurrencies.fiat)
  }, [
    defaultCurrencies.fiat,
    portfolioPriceHistory,
    isZeroBalance
  ])

  const visiblePortfolioNetworks = React.useMemo(() => {
    return networks.filter(
      (network) => !filteredOutPortfolioNetworkKeys
        .includes(networkEntityAdapter
          .selectId(network).toString())
    )
  }, [networks, filteredOutPortfolioNetworkKeys])

  const isPortfolioDown = Number(percentageChange) < 0

  // methods
  const onChangeTimeline = React.useCallback(
    (id: BraveWallet.AssetPriceTimeframe) => {
      window.localStorage.setItem(
        LOCAL_STORAGE_KEYS.PORTFOLIO_TIME_LINE_OPTION,
        id.toString()
      )
      dispatch(WalletActions.selectPortfolioTimeline(id))
    }, [])

  const onSelectAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    if (asset.contractAddress === '') {
      history.push(
        `${WalletRoutes.PortfolioAssets //
        }/${asset.chainId //
        }/${asset.symbol}`
      )
      return
    }
    if (asset.isErc721 || asset.isNft || asset.isErc1155) {
      history.push(
        `${WalletRoutes.PortfolioNFTs //
        }/${asset.chainId //
        }/${asset.contractAddress //
        }/${asset.tokenId}`
      )
    } else {
      history.push(
        `${WalletRoutes.PortfolioAssets //
        }/${asset.chainId //
        }/${asset.contractAddress}`
      )
    }
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
      <Column
        fullWidth={true}
        justifyContent='flex-start'
        margin={
          hidePortfolioNFTsTab
            ? '0px 0px 15px 0px'
            : '0px'
        }
      >
        <BalanceAndButtonsWrapper
          fullWidth={true}
          alignItems='center'
          padding='40px 32px'
        >
          <BalanceAndChangeWrapper>
            {formattedFullPortfolioFiatBalance !== '' ? (
              <BalanceText>
                {hidePortfolioBalances
                  ? '******'
                  : formattedFullPortfolioFiatBalance
                }
              </BalanceText>
            ) : (
              <Column padding='9px 0px'>
                <LoadingSkeleton width={150} height={36} />
              </Column>
            )}
            <Row
              alignItems='center'
              justifyContent='center'
              width='unset'
            >
              {formattedFullPortfolioFiatBalance !== '' ? (
                <>
                  <FiatChange
                    isDown={isPortfolioDown}
                  >
                    {hidePortfolioBalances
                      ? '*****'
                      : `${isPortfolioDown
                        ? ''
                        : '+'}${fiatValueChange}`
                    }
                  </FiatChange>
                  <PercentBubble
                    isDown={isPortfolioDown}
                  >
                    {hidePortfolioBalances
                      ? '*****'
                      : `${isPortfolioDown
                        ? ''
                        : '+'}${percentageChange}%`
                    }
                  </PercentBubble>
                </>
              ) : (
                <>
                  <LoadingSkeleton width={55} height={24} />
                  <HorizontalSpace space='8px' />
                  <LoadingSkeleton width={55} height={24} />
                </>
              )}
            </Row>
          </BalanceAndChangeWrapper>
          <BuySendSwapDepositNav />
        </BalanceAndButtonsWrapper>
        <ColumnReveal hideContent={hidePortfolioGraph}>
          <SelectTimelineWrapper
            padding='0px 32px'
            marginBottom={8}
          >
            <LineChartControls
              onSelectTimeline={onChangeTimeline}
              selectedTimeline={selectedPortfolioTimeline}
            />
          </SelectTimelineWrapper>
          <PortfolioOverviewChart
            hasZeroBalance={isZeroBalance}
          />
        </ColumnReveal>
      </Column>

      {!hidePortfolioNFTsTab &&
        <ControlsRow padding='24px 0px'>
          <SegmentedControl
            navOptions={PortfolioNavOptions}
            width={384}
          />
        </ControlsRow>
      }

      <Switch>
        <Route path={WalletRoutes.PortfolioAssets} exact>
          <TokenLists
            userAssetList={userAssetList}
            estimatedItemSize={58}
            horizontalPadding={20}
            onShowPortfolioSettings={() => setShowPortfolioSettings(true)}
            hideSmallBalances={hidePortfolioSmallBalances}
            networks={visiblePortfolioNetworks}
            accounts={usersFilteredAccounts}
            isPortfolio
            renderToken={({ item, account }) =>
              <PortfolioAssetItem
                action={() => onSelectAsset(item.asset)}
                key={getAssetIdKey(item.asset)}
                assetBalance={
                  selectedGroupAssetsByItem ===
                    AccountsGroupByOption.id
                    ? getBalance(account, item.asset)
                    : item.assetBalance
                }
                token={item.asset}
                hideBalances={hidePortfolioBalances}
                spotPrice={
                  spotPriceRegistry &&
                    !isLoadingSpotPrices &&
                    !isFetchingSpotPrices
                      ? getTokenPriceAmountFromRegistry(
                          spotPriceRegistry,
                          item.asset
                        )
                      : Amount.empty()
                }
              />
            }
          />
        </Route>

        <Route path={WalletRoutes.PortfolioNFTs} exact>
          <NftView
            onToggleShowIpfsBanner={onToggleShowIpfsBanner}
          />
        </Route>

        <Route
          path={WalletRoutes.Portfolio}
          exact={true}
          render={() => <Redirect to={WalletRoutes.PortfolioAssets} />
          }
        />

      </Switch>

      {showPortfolioSettings &&
        <PortfolioFiltersModal
          onClose={() => setShowPortfolioSettings(false)}
        />
      }
    </>
  )
}

export default PortfolioOverview
