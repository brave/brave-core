// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Constants
import {
  HIDE_SMALL_BALANCES_FIAT_THRESHOLD
} from '../../../../../../common/constants/magics'

// Selectors
import {
  useSafeUISelector,
  useSafeWalletSelector
} from '../../../../../../common/hooks/use-safe-selector'
import {
  UISelectors,
  WalletSelectors
} from '../../../../../../common/selectors'

// Types
import {
  BraveWallet,
  SpotPriceRegistry,
  UserAssetInfoType,
  WalletRoutes
} from '../../../../../../constants/types'
import { RenderTokenFunc } from './virtualized-tokens-list'

// Options
import { AssetFilterOptions, HighToLowAssetsFilterOption } from '../../../../../../options/asset-filter-options'
import {
  AccountsGroupByOption,
  NetworksGroupByOption
} from '../../../../../../options/group-assets-by-options'

// Utils
import Amount from '../../../../../../utils/amount'
import { getLocale } from '../../../../../../../common/locale'
import {
  networkEntityAdapter
} from '../../../../../../common/slices/entities/network.entity'
import { computeFiatAmount } from '../../../../../../utils/pricing-utils'
import {
  emptyNetwork,
  networkSupportsAccount
} from '../../../../../../utils/network-utils'
import { getBalance } from '../../../../../../utils/balance-utils'
import {
  getIsRewardsAccount,
  getIsRewardsNetwork,
  getIsRewardsToken
} from '../../../../../../utils/rewards_utils'

// Components
import SearchBar from '../../../../../shared/search-bar/index'
import NetworkFilterSelector from '../../../../network-filter-selector/index'
import { PortfolioAssetItemLoadingSkeleton } from '../../../../portfolio-asset-item/portfolio-asset-item-loading-skeleton'
import {
  AssetGroupContainer
} from '../../../../asset-group-container/asset-group-container'
import {
  EmptyTokenListState
} from '../empty-token-list-state/empty-token-list-state'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetExternalRewardsWalletQuery
} from '../../../../../../common/slices/api.slice'
import {
  TokenBalancesRegistry
} from '../../../../../../common/slices/entities/token-balance.entity'

// Styled Components
import {
  Column,
  Row,
  ScrollableColumn,
  Text
} from '../../../../../shared/style'
import {
  FilterTokenRow,
  CircleButton,
  ButtonIcon,
  SearchBarWrapper,
  ControlBarWrapper,
  SearchButtonWrapper,
  ContentWrapper
} from '../../style'

interface Props {
  userAssetList: UserAssetInfoType[]
  networks?: BraveWallet.NetworkInfo[]
  accounts?: BraveWallet.AccountInfo[]
  renderToken: RenderTokenFunc<UserAssetInfoType>
  enableScroll?: boolean
  maxListHeight?: string
  estimatedItemSize: number
  horizontalPadding?: number
  hideSmallBalances?: boolean
  isPortfolio?: boolean
  isV2?: boolean
  onShowPortfolioSettings?: () => void
  tokenBalancesRegistry: TokenBalancesRegistry | undefined
  spotPriceRegistry: SpotPriceRegistry | undefined
}

export const TokenLists = ({
  userAssetList,
  networks,
  accounts,
  renderToken,
  enableScroll,
  maxListHeight,
  horizontalPadding,
  hideSmallBalances,
  isPortfolio,
  isV2,
  onShowPortfolioSettings,
  tokenBalancesRegistry,
  spotPriceRegistry
}: Props) => {
  // routing
  const history = useHistory()

  // unsafe selectors
  const selectedAssetFilter = useSafeWalletSelector(WalletSelectors.selectedAssetFilter)

  // safe selectors
  const assetAutoDiscoveryCompleted = useSafeWalletSelector(WalletSelectors.assetAutoDiscoveryCompleted)
  const selectedGroupAssetsByItem =
    useSafeWalletSelector(WalletSelectors.selectedGroupAssetsByItem)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // queries
  const { data: externalRewardsInfo } = useGetExternalRewardsWalletQuery()

  // computed
  const externalRewardsProvider =
    externalRewardsInfo?.provider ?? undefined

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showSearchBar, setShowSearchBar] = React.useState<boolean>(false)

  // methods

  // This filters a list of assets when the user types in search bar
  const onSearchValueChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setSearchValue(event.target.value)
  }, [])

  const showAddAssetsModal = React.useCallback(() => {
    history.push(WalletRoutes.AddAssetModal)
  }, [])

  // memos
  const visibleTokens = React.useMemo(() => {
    return userAssetList.filter((asset) => asset.asset.visible)
  }, [userAssetList])

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const filteredOutSmallBalanceTokens = React.useMemo(() => {
    if (hideSmallBalances) {
      return visibleTokens.filter(
        (token) =>
          computeFiatAmount({
            spotPriceRegistry,
            value: token.assetBalance,
            token: token.asset
          }).gt(HIDE_SMALL_BALANCES_FIAT_THRESHOLD)
      )
    }
    return visibleTokens
  }, [visibleTokens, hideSmallBalances, spotPriceRegistry])

  const fungibleTokens = React.useMemo(() => {
    return filteredOutSmallBalanceTokens
      .filter(
        (token) => !(
          token.asset.isErc721 ||
          token.asset.isNft ||
          token.asset.isErc1155
        )
      )
  },
    [filteredOutSmallBalanceTokens]
  )

  const assetFilterItemInfo = React.useMemo(() => {
    return AssetFilterOptions.find(item => item.id === selectedAssetFilter) ?? HighToLowAssetsFilterOption
  }, [selectedAssetFilter])


  const filteredAssetList = React.useMemo(() => {
    const listToUse = isPortfolio
      ? fungibleTokens
      : userAssetList
    if (searchValue === '') {
      return listToUse
        .filter((asset) => asset.asset.visible)
    }
    return listToUse.filter((item) => {
      return (
        item.asset.name.toLowerCase() === searchValue.toLowerCase() ||
        item.asset.name.toLowerCase().startsWith(searchValue.toLowerCase()) ||
        item.asset.symbol.toLocaleLowerCase() === searchValue.toLowerCase() ||
        item.asset.symbol.toLowerCase().startsWith(searchValue.toLowerCase())
      )
    })
  }, [
    searchValue,
    fungibleTokens,
    isPortfolio,
    userAssetList
  ])

  // Returns a sorted list of assets based on the users
  // sort by pref.
  const getSortedFungibleTokensList = React.useCallback(
    (tokens: UserAssetInfoType[]) => {
      if (
        assetFilterItemInfo.id === 'highToLow' ||
        assetFilterItemInfo.id === 'lowToHigh'
      ) {
        return [...tokens].sort(function (a, b) {
          const aBalance = a.assetBalance
          const bBalance = b.assetBalance

          const bFiatBalance = computeFiatAmount({
            spotPriceRegistry,
            value: bBalance,
            token: b.asset
          })

          const aFiatBalance = computeFiatAmount({
            spotPriceRegistry,
            value: aBalance,
            token: a.asset
          })

          return assetFilterItemInfo.id === 'highToLow'
            ? bFiatBalance.minus(aFiatBalance).toNumber()
            : aFiatBalance.minus(bFiatBalance).toNumber()
        })
      }
      if (
        assetFilterItemInfo.id === 'aToZ' ||
        assetFilterItemInfo.id === 'zToA'
      ) {
        return [...tokens].sort(function (a, b) {
          const aName = a.asset.name.toUpperCase()
          const bName = b.asset.name.toUpperCase()
          return assetFilterItemInfo.id === 'aToZ'
            ? aName.localeCompare(bName)
            : bName.localeCompare(aName)
        })
      }
      return tokens
    }, [
    assetFilterItemInfo.id,
    spotPriceRegistry
  ])

  // Returns a list of assets based on provided network
  const getAssetsByNetwork = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      return getSortedFungibleTokensList(filteredAssetList)
        .filter(
          (asset) =>
            networkEntityAdapter
              .selectId(
                {
                  chainId: asset.asset.chainId,
                  coin: asset.asset.coin
                }).toString() ===
            networkEntityAdapter
              .selectId(network).toString()
        )
    }, [
    filteredAssetList,
    getSortedFungibleTokensList
  ])

  // Returns the full fiat value of provided network
  const getNetworkFiatValue = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      // Return an empty string to display a loading
      // skeleton while assets are populated.
      if (filteredAssetList.length === 0) {
        return Amount.empty()
      }
      const networksAssets = getAssetsByNetwork(network)
      // Return a 0 balance if the network has no
      // assets to display.
      if (networksAssets.length === 0) {
        return new Amount(0)
      }

      const amounts = networksAssets
        .map((asset) => computeFiatAmount({
          spotPriceRegistry,
          value: asset.assetBalance,
          token: asset.asset
        }))

      const reducedAmounts =
        amounts.reduce(function (a, b) {
          return a.plus(b)
        })

      return !reducedAmounts.isUndefined()
        ? reducedAmounts
        : Amount.empty()
    }, [
    computeFiatAmount,
    getAssetsByNetwork,
    filteredAssetList,
    spotPriceRegistry
  ])

  // Returns a list of assets based on provided coin type
  const getAssetsByCoin = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      const isRewardsAccount = getIsRewardsAccount(account.accountId)
      if (isRewardsAccount) {
        return filteredAssetList
          .filter(
            (asset) => getIsRewardsToken(asset.asset))
      }
      return filteredAssetList.filter((asset) => {
        const networkInfo = networks?.find(
          (network) =>
            network.coin === asset.asset.coin &&
            network.chainId === asset.asset.chainId
        )
        return (
          networkInfo && networkSupportsAccount(networkInfo, account.accountId)
        )
      })
    },
    [filteredAssetList, networks]
  )

  // Returns a list of assets based on provided account
  // and filters out small balances if hideSmallBalances
  // is enabled.
  const getFilteredOutAssetsByAccount = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      const isRewardsAccount = getIsRewardsAccount(account.accountId)
      if (hideSmallBalances) {
        return getAssetsByCoin(account).filter((token) =>
          computeFiatAmount({
            spotPriceRegistry,
            value:
              isRewardsAccount
                ? token.assetBalance
                : getBalance(
                  account.accountId,
                  token.asset,
                  tokenBalancesRegistry
                ),
            token: token.asset
          }).gt(HIDE_SMALL_BALANCES_FIAT_THRESHOLD)
        )
      }

      return getAssetsByCoin(account)
    }, [
    getAssetsByCoin,
    hideSmallBalances,
    spotPriceRegistry,
    tokenBalancesRegistry
  ])

  // Returns a new list of assets with the accounts
  // balance for each token.
  const getAccountsAssetBalances = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      const isRewardsAccount = getIsRewardsAccount(account.accountId)
      return getFilteredOutAssetsByAccount(account).map((asset) => ({
        ...asset,
        assetBalance:
          isRewardsAccount
            ? asset.assetBalance
            : getBalance(
              account.accountId,
              asset.asset,
              tokenBalancesRegistry
            )
      }))
    }, [getFilteredOutAssetsByAccount, tokenBalancesRegistry])

  // Returns a sorted assets list for an account.
  const getSortedAssetsByAccount = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      const accountsAssets = getAccountsAssetBalances(account)
      return getSortedFungibleTokensList(accountsAssets)
    }, [
    getAccountsAssetBalances,
    getSortedFungibleTokensList
  ])

  // Returns the full fiat value of provided account
  const getAccountFiatValue = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      // Return an empty string to display a loading
      // skeleton while assets are populated.
      if (filteredAssetList.length === 0) {
        return Amount.empty()
      }
      const accountsAssets = getSortedAssetsByAccount(account)
      // Return a 0 balance if the account has no
      // assets to display.
      if (
        accountsAssets
          .length === 0
      ) {
        return new Amount(0)
      }

      const amounts =
        accountsAssets
          .map((asset) => {
            return computeFiatAmount({
              spotPriceRegistry,
              value: asset.assetBalance,
              token: asset.asset
            })
          })

      const reducedAmounts =
        amounts.reduce(function (a, b) {
          return a.plus(b)
        })

      return !reducedAmounts.isUndefined()
        ? reducedAmounts
        : Amount.empty()
    }, [
    getSortedAssetsByAccount,
    filteredAssetList
  ])

  const onCloseSearchBar = React.useCallback(() => {
    setShowSearchBar(false)
    setSearchValue('')
  }, [])

  const noNetworks = !networks || networks.length === 0
  const noAccounts = !accounts || accounts.length === 0
  const showEmptyState = noNetworks || noAccounts

  const sortedNetworksWithBalances = React.useMemo(() => {
    if (noNetworks) {
      return undefined
    }
    return [...networks].filter((network) => {
      return getNetworkFiatValue(network).gt(0)
    }).sort((a, b) => {
      const aBalance = getNetworkFiatValue(a)
      const bBalance = getNetworkFiatValue(b)
      return bBalance.minus(aBalance).toNumber()
    })
  }, [
    networks,
    noNetworks,
    getNetworkFiatValue
  ])

  const listUiByNetworks = React.useMemo(() => {
    if (showEmptyState) {
      return <EmptyTokenListState />
    }
    if (
      !sortedNetworksWithBalances ||
      sortedNetworksWithBalances.length === 0
    ) {
      return <AssetGroupContainer
        balance=''
        isSkeleton={true}
        isDisabled={true}
        network={emptyNetwork}
      />
    }
    return sortedNetworksWithBalances.map((network) => {
      const networksFiatValue = getNetworkFiatValue(network)
      const isRewardsNetwork = getIsRewardsNetwork(network)
      const networksAssets = getAssetsByNetwork(network)
      return <AssetGroupContainer
        key={networkEntityAdapter
          .selectId(network).toString()}
        balance={
          networksFiatValue
            .isUndefined()
            ? ''
            : networksFiatValue
              .formatAsFiat(defaultFiatCurrency)
        }
        network={network}
        isDisabled={networksAssets.length === 0}
        externalProvider={
          isRewardsNetwork
            ? externalRewardsProvider
            : undefined
        }
      >
        {networksAssets
          .map(
            (token, index) =>
              renderToken(
                { index, item: token, viewMode: 'list' }
              )
          )
        }
        {!assetAutoDiscoveryCompleted && !isRewardsNetwork &&
          <PortfolioAssetItemLoadingSkeleton />
        }
      </AssetGroupContainer>
    })
  }, [
    getAssetsByNetwork,
    getNetworkFiatValue,
    renderToken,
    showEmptyState,
    sortedNetworksWithBalances,
    defaultFiatCurrency,
    assetAutoDiscoveryCompleted,
    externalRewardsProvider
  ])

  const sortedAccountsWithBalances = React.useMemo(() => {
    if (noAccounts) {
      return undefined
    }
    return [...accounts].filter((account) => {
      return getAccountFiatValue(account).gt(0)
    }).sort((a, b) => {
      const aBalance = getAccountFiatValue(a)
      const bBalance = getAccountFiatValue(b)
      return bBalance.minus(aBalance).toNumber()
    })
  }, [
    accounts,
    noAccounts,
    getAccountFiatValue
  ])

  const listUiByAccounts = React.useMemo(() => {
    if (showEmptyState) {
      return <EmptyTokenListState />
    }
    if (
      !sortedAccountsWithBalances ||
      sortedAccountsWithBalances.length === 0
    ) {
      return <AssetGroupContainer
        balance=''
        isSkeleton={true}
        isDisabled={true}
      />
    }
    return sortedAccountsWithBalances.map((account) => {
      const accountsFiatValue = getAccountFiatValue(account)
      const isRewardsAccount = getIsRewardsAccount(account.accountId)
      const accountsAssets = getSortedAssetsByAccount(account)
      return <AssetGroupContainer
        key={account.accountId.uniqueKey}
        balance={
          accountsFiatValue
            .isUndefined()
            ? ''
            : accountsFiatValue
              .formatAsFiat(defaultFiatCurrency)
        }
        account={account}
        isDisabled={accountsAssets.length === 0}
        externalProvider={
          isRewardsAccount
            ? externalRewardsProvider
            : undefined
        }
      >
        {accountsAssets
          .map(
            (token, index) =>
              renderToken(
                { index, item: token, viewMode: 'list', account }
              )
          )
        }
        {!assetAutoDiscoveryCompleted && !isRewardsAccount &&
          <PortfolioAssetItemLoadingSkeleton />
        }
      </AssetGroupContainer>
    })
  }, [
    getSortedAssetsByAccount,
    getAccountFiatValue,
    renderToken,
    showEmptyState,
    defaultFiatCurrency,
    assetAutoDiscoveryCompleted,
    sortedAccountsWithBalances,
    externalRewardsProvider
  ])

  const listUi = React.useMemo(() => {
    if (isPortfolio) {
      return selectedGroupAssetsByItem === NetworksGroupByOption.id
        ? listUiByNetworks
        : selectedGroupAssetsByItem === AccountsGroupByOption.id
          ? listUiByAccounts
          : noNetworks
            ? <EmptyTokenListState />
            : <>
              {
                getSortedFungibleTokensList(filteredAssetList)
                  .map((token, index) =>
                    renderToken(
                      { index, item: token, viewMode: 'list' }
                    )
                  )
              }
              {!assetAutoDiscoveryCompleted &&
                <PortfolioAssetItemLoadingSkeleton />
              }
            </>
    }
    return <>
      {
        filteredAssetList
          .map(
            (token, index) =>
              renderToken(
                { index, item: token, viewMode: 'list' }
              )
          )
      }
    </>
  }, [
    selectedGroupAssetsByItem,
    filteredAssetList,
    listUiByNetworks,
    listUiByAccounts,
    renderToken,
    assetAutoDiscoveryCompleted,
    isPortfolio,
    getSortedFungibleTokensList,
    noNetworks
  ])

  // render
  return (
    <ContentWrapper
      fullWidth={true}
      fullHeight={isPanel}
      justifyContent='flex-start'
      isPanel={isPanel}
    >
      {!isPortfolio &&
        <FilterTokenRow
          horizontalPadding={horizontalPadding}
          isV2={isV2}
        >
          <Column flex={1} style={{ minWidth: '25%' }} alignItems='flex-start'>
            <SearchBar
              placeholder={getLocale('braveWalletSearchText')}
              action={onSearchValueChange}
              value={searchValue}
              isV2={isV2}
            />
          </Column>
          <NetworkFilterSelector
            networkListSubset={networks}
            isV2={isV2}
          />
        </FilterTokenRow>
      }

      {isPortfolio &&
        <ControlBarWrapper
          justifyContent='space-between'
          alignItems='center'
          showSearchBar={showSearchBar}
        >
          {!showSearchBar &&
            <Text
              textSize='16px'
              isBold={true}
            >
              {getLocale('braveWalletAccountsAssets')}
            </Text>
          }
          <Row
            width={showSearchBar ? '100%' : 'unset'}
          >
            {!showEmptyState &&
              <SearchBarWrapper
                margin='0px 12px 0px 0px'
                showSearchBar={showSearchBar}
              >
                <SearchBar
                  placeholder={getLocale('braveWalletSearchText')}
                  action={onSearchValueChange}
                  value={searchValue}
                  isV2={isV2}
                />
              </SearchBarWrapper>
            }
            {showSearchBar &&
              <Row
                width='unset'
              >
                <CircleButton
                  onClick={onCloseSearchBar}
                >
                  <ButtonIcon name='close' />
                </CircleButton>
              </Row>
            }
            {!showSearchBar &&
              <Row
                width='unset'
              >
                {!showEmptyState &&
                  <SearchButtonWrapper
                    width='unset'
                  >
                    <CircleButton
                      marginRight={12}
                      onClick={() => setShowSearchBar(true)}
                    >
                      <ButtonIcon name='search' />
                    </CircleButton>
                  </SearchButtonWrapper>
                }
                <CircleButton
                  marginRight={12}
                  onClick={showAddAssetsModal}
                >
                  <ButtonIcon name='list-settings' />
                </CircleButton>

                <CircleButton
                  onClick={onShowPortfolioSettings}
                >
                  <ButtonIcon name='filter-settings' />
                </CircleButton>
              </Row>
            }
          </Row>
        </ControlBarWrapper>
      }

      {enableScroll
        ? (
          <ScrollableColumn
            maxHeight={maxListHeight}
            padding={
              `0px ${horizontalPadding !== undefined
                ? horizontalPadding
                : 0}px`
            }
          >
            {listUi}
          </ScrollableColumn>
        ) : (
          <Column
            fullWidth={true}
            padding='0px 24px 24px 24px'
          >
            {listUi}
          </Column>
        )
      }
    </ContentWrapper>
  )
}
