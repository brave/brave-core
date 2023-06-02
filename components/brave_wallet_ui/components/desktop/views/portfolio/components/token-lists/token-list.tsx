// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
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
  WalletRoutes
} from '../../../../../../constants/types'
import { RenderTokenFunc } from './virtualized-tokens-list'

// Options
import { AssetFilterOptions, HighToLowAssetsFilterOption } from '../../../../../../options/asset-filter-options'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import SearchBar from '../../../../../shared/search-bar/index'
import NetworkFilterSelector from '../../../../network-filter-selector/index'
import { PortfolioAssetItemLoadingSkeleton } from '../../../../portfolio-asset-item/portfolio-asset-item-loading-skeleton'

// Hooks
import usePricing from '../../../../../../common/hooks/pricing'

// Styled Components
import {
  Column,
  Row,
  ScrollableColumn,
  Text,
  VerticalSpace
} from '../../../../../shared/style'
import {
  FilterTokenRow,
  CircleButton,
  ButtonIcon
} from '../../style'

interface Props {
  userAssetList: UserAssetInfoType[]
  networks?: BraveWallet.NetworkInfo[]
  renderToken: RenderTokenFunc
  hideAutoDiscovery?: boolean
  enableScroll?: boolean
  maxListHeight?: string
  estimatedItemSize: number
  horizontalPadding?: number
  hideSmallBalances?: boolean
  isPortfolio?: boolean
  onShowPortfolioSettings?: () => void
}

export const TokenLists = ({
  userAssetList,
  networks,
  renderToken,
  enableScroll,
  maxListHeight,
  hideAutoDiscovery,
  horizontalPadding,
  hideSmallBalances,
  isPortfolio,
  onShowPortfolioSettings
}: Props) => {
  // routing
  const history = useHistory()
  const { tokenId } = useParams<{ tokenId?: string }>()

  // unsafe selectors
  const tokenSpotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
  const selectedAssetFilter = useSafeWalletSelector(WalletSelectors.selectedAssetFilter)

  // safe selectors
  const assetAutoDiscoveryCompleted = useSafeWalletSelector(WalletSelectors.assetAutoDiscoveryCompleted)

  // hooks
  const { computeFiatAmount } = usePricing(tokenSpotPrices)

  // state
  const [searchValue, setSearchValue] = React.useState<string>(tokenId ?? '')

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

  const filteredOutSmallBalanceTokens = React.useMemo(() => {
    if (hideSmallBalances) {
      return visibleTokens.filter(
        (token) =>
          computeFiatAmount(
            token.assetBalance,
            token.asset.symbol,
            token.asset.decimals,
            token.asset.contractAddress,
            token.asset.chainId)
            .gt(HIDE_SMALL_BALANCES_FIAT_THRESHOLD)
      )
    }
    return visibleTokens
  }, [visibleTokens, hideSmallBalances, computeFiatAmount])

  const filteredAssetList = React.useMemo(() => {
    if (searchValue === '') {
      return filteredOutSmallBalanceTokens
        .filter((asset) => asset.asset.visible)
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

  const fungibleTokens = React.useMemo(() => {
    return filteredAssetList
      .filter(
        (token) => !(
          token.asset.isErc721 ||
          token.asset.isNft ||
          token.asset.isErc1155
        )
      )
  },
    [filteredAssetList]
  )

  const assetFilterItemInfo = React.useMemo(() => {
    return AssetFilterOptions.find(item => item.id === selectedAssetFilter) ?? HighToLowAssetsFilterOption
  }, [selectedAssetFilter])

  const sortedFungibleTokensList: UserAssetInfoType[] = React.useMemo(() => {
    if (
      assetFilterItemInfo.id === 'highToLow' ||
      assetFilterItemInfo.id === 'lowToHigh'
    ) {
      return [...fungibleTokens].sort(function (a, b) {
        const aBalance = a.assetBalance
        const bBalance = b.assetBalance
        const bFiatBalance = computeFiatAmount(bBalance, b.asset.symbol, b.asset.decimals, b.asset.contractAddress, b.asset.chainId)
        const aFiatBalance = computeFiatAmount(aBalance, a.asset.symbol, a.asset.decimals, a.asset.contractAddress, a.asset.chainId)
        return assetFilterItemInfo.id === 'highToLow'
          ? bFiatBalance.toNumber() - aFiatBalance.toNumber()
          : aFiatBalance.toNumber() - bFiatBalance.toNumber()
      })
    }
    if (
      assetFilterItemInfo.id === 'aToZ' ||
      assetFilterItemInfo.id === 'zToA'
    ) {
      return [...fungibleTokens].sort(function (a, b) {
        const aName = a.asset.name.toUpperCase()
        const bName = b.asset.name.toUpperCase()
        return assetFilterItemInfo.id === 'aToZ'
          ? aName.localeCompare(bName)
          : bName.localeCompare(aName)
      })
    }
    return fungibleTokens
  }, [
    assetFilterItemInfo.id,
    fungibleTokens,
    computeFiatAmount
  ])

  const listUi = React.useMemo(() => {
    return <>
      {sortedFungibleTokensList.map((token, index) => renderToken({ index, item: token, viewMode: 'list' }))}
      {!assetAutoDiscoveryCompleted && !hideAutoDiscovery &&
        <PortfolioAssetItemLoadingSkeleton />
      }
    </>
  }, [
    sortedFungibleTokensList,
    renderToken,
    assetAutoDiscoveryCompleted,
    hideAutoDiscovery
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
        <FilterTokenRow horizontalPadding={horizontalPadding}>
          <Column flex={1} style={{ minWidth: '25%' }} alignItems='flex-start'>
            <SearchBar
              placeholder={getLocale('braveWalletSearchText')}
              action={onSearchValueChange}
              value={searchValue}
            />
          </Column>
          <NetworkFilterSelector networkListSubset={networks} />
        </FilterTokenRow>
      }

      {isPortfolio &&
        <Row
          justifyContent='space-between'
          alignItems='center'
          padding='0px 18px'
          marginBottom={16}
        >
          <Text
            textSize='16px'
            isBold={true}
          >
            {getLocale('braveWalletAccountsAssets')}
          </Text>
          <Row
            width='unset'
          >
            <Row
              style={{ width: 230 }}
              margin='0px 12px 0px 0px'
            >
              <SearchBar
                placeholder={getLocale('braveWalletSearchText')}
                action={onSearchValueChange}
                value={searchValue}
                isV2={true}
              />
            </Row>
            <Row
              width='unset'
            >
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
          </Row>
        </Row>
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
        )
        : listUi
      }

      {isPortfolio &&
        <VerticalSpace space='18px' />
      }
    </>
  )
}
