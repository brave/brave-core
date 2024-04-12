// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Constants
import {
  HIDE_SMALL_BALANCES_FIAT_THRESHOLD //
} from '../../../../../../common/constants/magics'
import {
  emptyRewardsInfo //
} from '../../../../../../common/async/base-query-cache'
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../../../common/constants/local-storage-keys'

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
import {
  AssetFilterOptions,
  HighToLowAssetsFilterOption
} from '../../../../../../options/asset-filter-options'
import {
  AccountsGroupByOption,
  NetworksGroupByOption,
  NoneGroupByOption
} from '../../../../../../options/group-assets-by-options'

// Utils
import Amount from '../../../../../../utils/amount'
import { getLocale } from '../../../../../../../common/locale'
import {
  networkEntityAdapter //
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
import {
  useLocalStorage //
} from '../../../../../../common/hooks/use_local_storage'

// Components
import SearchBar from '../../../../../shared/search-bar/index'
import { PortfolioAssetItemLoadingSkeleton } from '../../../../portfolio-asset-item/portfolio-asset-item-loading-skeleton'
import {
  AssetGroupContainer //
} from '../../../../asset-group-container/asset-group-container'
import {
  EmptyTokenListState //
} from '../empty-token-list-state/empty-token-list-state'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetRewardsInfoQuery
} from '../../../../../../common/slices/api.slice'
import {
  TokenBalancesRegistry //
} from '../../../../../../common/slices/entities/token-balance.entity'

// Styled Components
import { Row, ScrollableColumn, Text } from '../../../../../shared/style'
import {
  PortfolioActionButton,
  ButtonIcon,
  SearchBarWrapper,
  ControlBarWrapper,
  SearchButtonWrapper,
  ContentWrapper
} from '../../style'
import { FlatTokenListWrapper, GroupTokenListWrapper } from './token-list.style'

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
  onShowPortfolioSettings?: () => void
  tokenBalancesRegistry: TokenBalancesRegistry | undefined | null
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
  onShowPortfolioSettings,
  tokenBalancesRegistry,
  spotPriceRegistry
}: Props) => {
  // routing
  const history = useHistory()

  // Local-Storage
  const [selectedAssetFilter] = useLocalStorage<string>(
    LOCAL_STORAGE_KEYS.PORTFOLIO_ASSET_FILTER_OPTION,
    HighToLowAssetsFilterOption.id
  )
  const [selectedGroupAssetsByItem] = useLocalStorage<string>(
    LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_ASSETS_BY,
    NoneGroupByOption.id
  )

  // safe selectors
  const assetAutoDiscoveryCompleted = useSafeWalletSelector(
    WalletSelectors.assetAutoDiscoveryCompleted
  )
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: { provider: externalRewardsProvider } = emptyRewardsInfo } =
    useGetRewardsInfoQuery()

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showSearchBar, setShowSearchBar] = React.useState<boolean>(false)

  // methods

  // This filters a list of assets when the user types in search bar
  const onSearchValueChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
    },
    []
  )

  const showAddAssetsModal = React.useCallback(() => {
    history.push(WalletRoutes.AddAssetModal)
  }, [history])

  // memos

  const filteredOutSmallBalanceTokens = React.useMemo(() => {
    if (hideSmallBalances) {
      return userAssetList.filter((token) =>
        computeFiatAmount({
          spotPriceRegistry,
          value: token.assetBalance,
          token: token.asset
        }).gt(HIDE_SMALL_BALANCES_FIAT_THRESHOLD)
      )
    }
    return userAssetList
  }, [userAssetList, hideSmallBalances, spotPriceRegistry])

  const assetFilterItemInfo = React.useMemo(() => {
    return (
      AssetFilterOptions.find((item) => item.id === selectedAssetFilter) ??
      HighToLowAssetsFilterOption
    )
  }, [selectedAssetFilter])

  const filteredAssetList = React.useMemo(() => {
    if (searchValue === '') {
      return filteredOutSmallBalanceTokens
    }
    return filteredOutSmallBalanceTokens.filter((item) => {
      return (
        item.asset.name.toLowerCase() === searchValue.toLowerCase() ||
        item.asset.name.toLowerCase().startsWith(searchValue.toLowerCase()) ||
        item.asset.symbol.toLocaleLowerCase() === searchValue.toLowerCase() ||
        item.asset.symbol.toLowerCase().startsWith(searchValue.toLowerCase())
      )
    })
  }, [searchValue, filteredOutSmallBalanceTokens])

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
    },
    [assetFilterItemInfo.id, spotPriceRegistry]
  )

  // Returns a list of assets based on provided network
  const getAssetsByNetwork = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      return getSortedFungibleTokensList(filteredAssetList).filter(
        (asset) =>
          networkEntityAdapter
            .selectId({
              chainId: asset.asset.chainId,
              coin: asset.asset.coin
            })
            .toString() === networkEntityAdapter.selectId(network).toString()
      )
    },
    [filteredAssetList, getSortedFungibleTokensList]
  )

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

      const amounts = networksAssets.map((asset) =>
        computeFiatAmount({
          spotPriceRegistry,
          value: asset.assetBalance,
          token: asset.asset
        })
      )

      const reducedAmounts = amounts.reduce(function (a, b) {
        return a.plus(b)
      })

      return !reducedAmounts.isUndefined() ? reducedAmounts : Amount.empty()
    },
    [getAssetsByNetwork, filteredAssetList, spotPriceRegistry]
  )

  const doesNetworkHaveBalance = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      return getAssetsByNetwork(network).some((asset) =>
        new Amount(asset.assetBalance).gt(0)
      )
    },
    [getAssetsByNetwork]
  )

  // Returns a list of assets based on provided coin type
  const getAssetsByCoin = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      const isRewardsAccount = getIsRewardsAccount(account.accountId)
      if (isRewardsAccount) {
        return filteredAssetList.filter((asset) =>
          getIsRewardsToken(asset.asset)
        )
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
            value: isRewardsAccount
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
    },
    [
      getAssetsByCoin,
      hideSmallBalances,
      spotPriceRegistry,
      tokenBalancesRegistry
    ]
  )

  // Returns a new list of assets with the accounts
  // balance for each token.
  const getAccountsAssetBalances = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      const isRewardsAccount = getIsRewardsAccount(account.accountId)
      return getFilteredOutAssetsByAccount(account).map((asset) => ({
        ...asset,
        assetBalance: isRewardsAccount
          ? asset.assetBalance
          : getBalance(account.accountId, asset.asset, tokenBalancesRegistry)
      }))
    },
    [getFilteredOutAssetsByAccount, tokenBalancesRegistry]
  )

  // Returns a sorted assets list for an account.
  const getSortedAssetsByAccount = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      const accountsAssets = getAccountsAssetBalances(account)
      return getSortedFungibleTokensList(accountsAssets)
    },
    [getAccountsAssetBalances, getSortedFungibleTokensList]
  )

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
      if (accountsAssets.length === 0) {
        return new Amount(0)
      }

      const amounts = accountsAssets.map((asset) => {
        return computeFiatAmount({
          spotPriceRegistry,
          value: asset.assetBalance,
          token: asset.asset
        })
      })

      const reducedAmounts = amounts.reduce(function (a, b) {
        return a.plus(b)
      })

      return !reducedAmounts.isUndefined() ? reducedAmounts : Amount.empty()
    },
    [filteredAssetList.length, getSortedAssetsByAccount, spotPriceRegistry]
  )

  const doesAccountHaveBalance = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      return getFilteredOutAssetsByAccount(account).some((asset) => {
        return new Amount(asset.assetBalance).gt(0)
      })
    },
    [getFilteredOutAssetsByAccount]
  )

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
    if (hideSmallBalances) {
      return networks
        .filter((network) => {
          return getNetworkFiatValue(network).gt(0)
        })
        .sort((a, b) => {
          const aBalance = getNetworkFiatValue(a)
          const bBalance = getNetworkFiatValue(b)
          return bBalance.minus(aBalance).toNumber()
        })
    }
    return networks.filter(doesNetworkHaveBalance).sort((a, b) => {
      const aBalance = getNetworkFiatValue(a)
      const bBalance = getNetworkFiatValue(b)
      return bBalance.minus(aBalance).toNumber()
    })
  }, [
    networks,
    noNetworks,
    getNetworkFiatValue,
    hideSmallBalances,
    doesNetworkHaveBalance
  ])

  const listUiByNetworks = React.useMemo(() => {
    if (showEmptyState) {
      return <EmptyTokenListState />
    }
    if (
      !sortedNetworksWithBalances ||
      sortedNetworksWithBalances.length === 0
    ) {
      return (
        <AssetGroupContainer
          balance=''
          isSkeleton={true}
          isDisabled={true}
          network={emptyNetwork}
        />
      )
    }
    return sortedNetworksWithBalances.map((network) => {
      const networksFiatValue = getNetworkFiatValue(network)
      const isRewardsNetwork = getIsRewardsNetwork(network)
      const networksAssets = getAssetsByNetwork(network)
      return (
        <AssetGroupContainer
          key={networkEntityAdapter.selectId(network).toString()}
          balance={
            networksFiatValue.isUndefined()
              ? ''
              : networksFiatValue.formatAsFiat(defaultFiatCurrency)
          }
          network={network}
          isDisabled={networksAssets.length === 0}
          externalProvider={
            isRewardsNetwork ? externalRewardsProvider : undefined
          }
        >
          {networksAssets.map((token, index) =>
            renderToken({ index, item: token, viewMode: 'list' })
          )}
          {!assetAutoDiscoveryCompleted && !isRewardsNetwork && (
            <PortfolioAssetItemLoadingSkeleton />
          )}
        </AssetGroupContainer>
      )
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
    if (hideSmallBalances) {
      return accounts
        .filter((account) => {
          return getAccountFiatValue(account).gt(0)
        })
        .sort((a, b) => {
          const aBalance = getAccountFiatValue(a)
          const bBalance = getAccountFiatValue(b)
          return bBalance.minus(aBalance).toNumber()
        })
    }
    return accounts.filter(doesAccountHaveBalance).sort((a, b) => {
      const aBalance = getAccountFiatValue(a)
      const bBalance = getAccountFiatValue(b)
      return bBalance.minus(aBalance).toNumber()
    })
  }, [
    accounts,
    noAccounts,
    getAccountFiatValue,
    hideSmallBalances,
    doesAccountHaveBalance
  ])

  const listUiByAccounts = React.useMemo(() => {
    if (showEmptyState) {
      return <EmptyTokenListState />
    }
    if (
      !sortedAccountsWithBalances ||
      sortedAccountsWithBalances.length === 0
    ) {
      return (
        <AssetGroupContainer
          balance=''
          isSkeleton={true}
          isDisabled={true}
        />
      )
    }
    return sortedAccountsWithBalances.map((account) => {
      const accountsFiatValue = getAccountFiatValue(account)
      const isRewardsAccount = getIsRewardsAccount(account.accountId)
      const accountsAssets = getSortedAssetsByAccount(account)
      return (
        <AssetGroupContainer
          key={account.accountId.uniqueKey}
          balance={
            accountsFiatValue.isUndefined()
              ? ''
              : accountsFiatValue.formatAsFiat(defaultFiatCurrency)
          }
          account={account}
          isDisabled={accountsAssets.length === 0}
          externalProvider={
            isRewardsAccount ? externalRewardsProvider : undefined
          }
        >
          {accountsAssets.map((token, index) =>
            renderToken({ index, item: token, viewMode: 'list', account })
          )}
          {!assetAutoDiscoveryCompleted && !isRewardsAccount && (
            <PortfolioAssetItemLoadingSkeleton />
          )}
        </AssetGroupContainer>
      )
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
    return selectedGroupAssetsByItem === NetworksGroupByOption.id ? (
      <GroupTokenListWrapper fullWidth={true}>
        {listUiByNetworks}
      </GroupTokenListWrapper>
    ) : selectedGroupAssetsByItem === AccountsGroupByOption.id ? (
      <GroupTokenListWrapper fullWidth={true}>
        {listUiByAccounts}
      </GroupTokenListWrapper>
    ) : noNetworks ? (
      <EmptyTokenListState />
    ) : (
      <FlatTokenListWrapper fullWidth={true}>
        {getSortedFungibleTokensList(filteredAssetList).map((token, index) =>
          renderToken({ index, item: token, viewMode: 'list' })
        )}
        {!assetAutoDiscoveryCompleted && <PortfolioAssetItemLoadingSkeleton />}
      </FlatTokenListWrapper>
    )
  }, [
    selectedGroupAssetsByItem,
    filteredAssetList,
    listUiByNetworks,
    listUiByAccounts,
    renderToken,
    assetAutoDiscoveryCompleted,
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
      <ControlBarWrapper
        justifyContent='space-between'
        alignItems='center'
        showSearchBar={showSearchBar}
      >
        {!showSearchBar && (
          <Text
            textSize='16px'
            isBold={true}
          >
            {getLocale('braveWalletAccountsAssets')}
          </Text>
        )}
        <Row width={showSearchBar ? '100%' : 'unset'}>
          {!showEmptyState && (
            <SearchBarWrapper
              margin='0px 12px 0px 0px'
              showSearchBar={showSearchBar}
            >
              <SearchBar
                placeholder={getLocale('braveWalletSearchText')}
                action={onSearchValueChange}
                value={searchValue}
                isV2={true}
              />
            </SearchBarWrapper>
          )}
          {showSearchBar && (
            <Row width='unset'>
              <PortfolioActionButton onClick={onCloseSearchBar}>
                <ButtonIcon name='close' />
              </PortfolioActionButton>
            </Row>
          )}
          {!showSearchBar && (
            <Row
              width='unset'
              gap='12px'
            >
              {!showEmptyState && (
                <SearchButtonWrapper width='unset'>
                  <PortfolioActionButton onClick={() => setShowSearchBar(true)}>
                    <ButtonIcon name='search' />
                  </PortfolioActionButton>
                </SearchButtonWrapper>
              )}
              <PortfolioActionButton onClick={showAddAssetsModal}>
                <ButtonIcon name='list-settings' />
              </PortfolioActionButton>

              <PortfolioActionButton onClick={onShowPortfolioSettings}>
                <ButtonIcon name='filter-settings' />
              </PortfolioActionButton>
            </Row>
          )}
        </Row>
      </ControlBarWrapper>

      {enableScroll ? (
        <ScrollableColumn
          maxHeight={maxListHeight}
          padding={`0px ${
            horizontalPadding !== undefined ? horizontalPadding : 0
          }px`}
        >
          {listUi}
        </ScrollableColumn>
      ) : (
        listUi
      )}
    </ContentWrapper>
  )
}
