// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
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

// Hooks
import {
  useOnClickOutside
} from '../../../../common/hooks/useOnClickOutside'

// Constants
import {
  BraveWallet,
  UserAssetInfoType,
  SupportedTestNetworks,
  WalletRoutes
} from '../../../../constants/types'
import {
  LOCAL_STORAGE_KEYS
} from '../../../../common/constants/local-storage-keys'

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
import { PortfolioNavOptions } from '../../../../options/nav-options'

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
import {
  LineChartControlsMenu
} from '../../wallet-menus/line-chart-controls-menu'
import ColumnReveal from '../../../shared/animated-reveals/column-reveal'
import { NftView } from '../nfts/nft-view'

// Styled Components
import {
  BalanceText,
  PercentBubble,
  FiatChange,
  SelectTimelinButton,
  SelectTimelinButtonIcon,
  SelectTimelineWrapper,
  ControlsRow
} from './style'
import {
  Column,
  Row,
  HorizontalSpace
} from '../../../shared/style'
import { useGetVisibleNetworksQuery } from '../../../../common/slices/api.slice'

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
  const portfolioPriceHistory =
    useUnsafeWalletSelector(WalletSelectors.portfolioPriceHistory)


  // queries
  const { data: networks } = useGetVisibleNetworksQuery()

  // state
  const [showLineChartControlMenu, setShowLineChartControlMenu] =
    React.useState<boolean>(false)

  // refs
  const lineChartControlMenuRef = React.useRef<HTMLDivElement>(null)

  // hooks
  useOnClickOutside(
    lineChartControlMenuRef,
    () => setShowLineChartControlMenu(false),
    showLineChartControlMenu
  )

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
  const fullPortfolioFiatBalance = React.useMemo((): Amount => {
    if (visibleAssetOptions.length === 0) {
      return new Amount('')
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
    return grandTotal
  },
    [
      visibleAssetOptions,
      transactionSpotPrices
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
    if (portfolioPriceHistory.length !== 0) {
      const oldestValue =
        new Amount(portfolioPriceHistory[0].close)
      const difference =
        fullPortfolioFiatBalance.minus(oldestValue)
      return `${difference.div(oldestValue).times(100).format(2)}`
    }
    return '0.00'
  }, [portfolioPriceHistory, fullPortfolioFiatBalance])

  const fiatValueChange = React.useMemo(() => {
    if (portfolioPriceHistory.length !== 0) {
      const oldestValue =
        new Amount(portfolioPriceHistory[0].close)
      return fullPortfolioFiatBalance
        .minus(oldestValue)
        .formatAsFiat(defaultCurrencies.fiat)
    }
    return new Amount(0).formatAsFiat(defaultCurrencies.fiat)
  }, [defaultCurrencies.fiat, portfolioPriceHistory])

  const isPortfolioDown = Number(percentageChange) < 0

  // methods
  const onChangeTimeline = React.useCallback(
    (id: BraveWallet.AssetPriceTimeframe) => {
      window.localStorage.setItem(
        LOCAL_STORAGE_KEYS.PORTFOLIO_TIME_LINE_OPTION,
        id.toString()
      )
      dispatch(WalletActions.selectPortfolioTimeline(id))
      setShowLineChartControlMenu(false)
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
      >
        <Column
          fullWidth={true}
          alignItems='center'
          padding='40px 0px'
        >
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
        </Column>

        <ColumnReveal hideContent={hidePortfolioGraph}>
          <SelectTimelineWrapper
            ref={lineChartControlMenuRef}
          >
            <SelectTimelinButton
              onClick={() => setShowLineChartControlMenu(prev => !prev)}
            >
              {getLocale(
                ChartTimelineOptions[selectedPortfolioTimeline].name
              )}
              <SelectTimelinButtonIcon
                isOpen={showLineChartControlMenu}
                name='carat-down'
              />
            </SelectTimelinButton>
            {showLineChartControlMenu &&
              <LineChartControlsMenu
                onClick={onChangeTimeline}
              />
            }
          </SelectTimelineWrapper>
          <PortfolioOverviewChart
            hasZeroBalance={isZeroBalance}
          />
        </ColumnReveal>
      </Column>

      <ControlsRow padding='24px 0px'>
        <SegmentedControl
          navOptions={PortfolioNavOptions}
          width={384}
        />
      </ControlsRow>

      <Switch>
        <Route path={WalletRoutes.PortfolioAssets} exact>
          <TokenLists
            userAssetList={userAssetList}
            networks={networks || []}
            estimatedItemSize={58}
            horizontalPadding={20}
            renderToken={({ item }) =>
              <PortfolioAssetItem
                action={() => onSelectAsset(item.asset)}
                key={getAssetIdKey(item.asset)}
                assetBalance={item.assetBalance}
                token={item.asset}
                hideBalances={hidePortfolioBalances}
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

    </>
  )
}

export default PortfolioOverview
