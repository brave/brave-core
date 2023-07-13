// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useHistory, useParams } from 'react-router'

// Constants
import {
  HIDE_SMALL_BALANCES_FIAT_THRESHOLD
} from '../../../../../../common/constants/magics'

// Selectors
import { useSafeWalletSelector, useUnsafeWalletSelector } from '../../../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../../../common/selectors'

// Types
import {
  BraveWallet,
  UserAssetInfoType,
  WalletRoutes,
  WalletAccountType
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
import { getBalance } from '../../../../../../utils/balance-utils'
import { getPriceIdForToken } from '../../../../../../utils/api-utils'
import { computeFiatAmount } from '../../../../../../utils/pricing-utils'

// Components
import SearchBar from '../../../../../shared/search-bar/index'
import NetworkFilterSelector from '../../../../network-filter-selector/index'
import { PortfolioAssetItemLoadingSkeleton } from '../../../../portfolio-asset-item/portfolio-asset-item-loading-skeleton'
import {
  AssetGroupContainer
} from '../../../../asset-group-container/asset-group-container'

// Queries
import {
  useGetTokenSpotPricesQuery
} from '../../../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../../../../common/slices/constants'

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
  SearchButtonWrapper
} from '../../style'

interface Props {
  userAssetList: UserAssetInfoType[]
  networks?: BraveWallet.NetworkInfo[]
  accounts?: WalletAccountType[]
  renderToken: RenderTokenFunc
  enableScroll?: boolean
  maxListHeight?: string
  estimatedItemSize: number
  horizontalPadding?: number
  hideSmallBalances?: boolean
  isPortfolio?: boolean
  isV2?: boolean
  onShowPortfolioSettings?: () => void
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
  onShowPortfolioSettings
}: Props) => {
  // routing
  const history = useHistory()
  const { tokenId } = useParams<{ tokenId?: string }>()

  // unsafe selectors
  const selectedAssetFilter = useSafeWalletSelector(WalletSelectors.selectedAssetFilter)
  const defaultCurrencies =
    useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)

  // safe selectors
  const assetAutoDiscoveryCompleted = useSafeWalletSelector(WalletSelectors.assetAutoDiscoveryCompleted)
  const selectedGroupAssetsByItem =
    useSafeWalletSelector(WalletSelectors.selectedGroupAssetsByItem)

  // state
  const [searchValue, setSearchValue] = React.useState<string>(tokenId ?? '')
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

  const tokenPriceIds = React.useMemo(() =>
    visibleTokens
      .filter(({ assetBalance }) => new Amount(assetBalance).gt(0))
      .filter(({ asset }) =>
        !asset.isErc721 && !asset.isErc1155 && !asset.isNft)
      .map(token => getPriceIdForToken(token.asset)),
    [visibleTokens]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length ? { ids: tokenPriceIds } : skipToken,
    querySubscriptionOptions60s
  )

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
    defaultCurrencies.fiat,
    spotPriceRegistry
  ])

  // Returns a list of assets based on provided coin type
  const getAssetsByCoin = React.useCallback(
    (account: WalletAccountType) => {
      return filteredAssetList
        .filter(
          (asset) => asset.asset.coin ===
            account.accountId.coin
        )
    }, [filteredAssetList])

  // Returns a list of assets based on provided account
  // and filters out small balances if hideSmallBalances
  // is enabled.
  const getFilteredOutAssetsByAccount = React.useCallback(
    (account: WalletAccountType) => {
      if (hideSmallBalances) {
        return getAssetsByCoin(account).filter(
          (token) =>
            computeFiatAmount({
              spotPriceRegistry,
              value: getBalance(account, token.asset),
              token: token.asset
            }).gt(HIDE_SMALL_BALANCES_FIAT_THRESHOLD)
        )
      }

      return getAssetsByCoin(account)
    }, [
    getAssetsByCoin,
    hideSmallBalances,
    spotPriceRegistry
  ])

  // Returns a new list of assets with the accounts
  // balance for each token.
  const getAccountsAssetBalances = React.useCallback(
    (account: WalletAccountType) => {
      return getFilteredOutAssetsByAccount(account)
        .map((asset) => ({
          ...asset,
          assetBalance: getBalance(account, asset.asset)
        }))
    }, [getFilteredOutAssetsByAccount])

  // Returns a sorted assets list for an account.
  const getSortedAssetsByAccount = React.useCallback(
    (account: WalletAccountType) => {
      const accountsAssets = getAccountsAssetBalances(account)
      return getSortedFungibleTokensList(accountsAssets)
    }, [
    getAccountsAssetBalances,
    getSortedFungibleTokensList
  ])

  // Returns the full fiat value of provided account
  const getAccountFiatValue = React.useCallback(
    (account: WalletAccountType) => {
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

  const listUiByNetworks = React.useMemo(() => {
    if (!networks) {
      return undefined
    }
    return [...networks].sort((a, b) => {
      const aBalance = getNetworkFiatValue(a)
      const bBalance = getNetworkFiatValue(b)
      return bBalance.minus(aBalance).toNumber()
    })?.map((network) => {
      const networksFiatValue = getNetworkFiatValue(network)
      const networksAssets = getAssetsByNetwork(network)
      return <AssetGroupContainer
        key={networkEntityAdapter
          .selectId(network).toString()}
        balance={
          networksFiatValue
            .isUndefined()
            ? ''
            : networksFiatValue
              .formatAsFiat(defaultCurrencies.fiat)
        }
        network={network}
        isDisabled={networksAssets.length === 0}
      >
        {networksAssets
          .map(
            (token, index) =>
              renderToken(
                { index, item: token, viewMode: 'list' }
              )
          )
        }
        {!assetAutoDiscoveryCompleted &&
          <PortfolioAssetItemLoadingSkeleton />
        }
      </AssetGroupContainer>
    })
  }, [
    getAssetsByNetwork,
    getNetworkFiatValue,
    renderToken,
    networks,
    defaultCurrencies.fiat,
    assetAutoDiscoveryCompleted
  ])

  const listUiByAccounts = React.useMemo(() => {
    if (!accounts) {
      return undefined
    }
    return [...accounts].sort((a, b) => {
      const aBalance = getAccountFiatValue(a)
      const bBalance = getAccountFiatValue(b)
      return bBalance.minus(aBalance).toNumber()
    })?.map((account) => {
      const accountsFiatValue = getAccountFiatValue(account)
      const accountsAssets = getSortedAssetsByAccount(account)
      return <AssetGroupContainer
        key={account.accountId.uniqueKey}
        balance={
          accountsFiatValue
            .isUndefined()
            ? ''
            : accountsFiatValue
              .formatAsFiat(defaultCurrencies.fiat)
        }
        account={account}
        isDisabled={accountsAssets.length === 0}
      >
        {accountsAssets
          .map(
            (token, index) =>
              renderToken(
                { index, item: token, viewMode: 'list', account }
              )
          )
        }
        {!assetAutoDiscoveryCompleted &&
          <PortfolioAssetItemLoadingSkeleton />
        }
      </AssetGroupContainer>
    })
  }, [
    getSortedAssetsByAccount,
    getAccountFiatValue,
    renderToken,
    accounts,
    defaultCurrencies.fiat,
    assetAutoDiscoveryCompleted
  ])

  const listUi = React.useMemo(() => {
    if (isPortfolio) {
      return selectedGroupAssetsByItem === NetworksGroupByOption.id
        ? listUiByNetworks
        : selectedGroupAssetsByItem === AccountsGroupByOption.id
          ? listUiByAccounts
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
    getSortedFungibleTokensList
  ])

  // effects
  React.useEffect(() => {
    // reset search field on list update
    if (userAssetList && !tokenId) {
      setSearchValue('')
    }
  }, [userAssetList, tokenId])

  // render
  return (
    <>
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
    </>
  )
}
