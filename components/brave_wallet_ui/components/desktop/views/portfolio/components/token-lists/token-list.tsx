// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useParams } from 'react-router'

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
import AddButton from '../../../../add-button/index'
import NetworkFilterSelector from '../../../../network-filter-selector/index'
import { AccountFilterSelector } from '../../../../account-filter-selector/account-filter-selector'
import { AssetFilterSelector } from '../../../../asset-filter-selector/asset-filter-selector'
import { PortfolioAssetItemLoadingSkeleton } from '../../../../portfolio-asset-item/portfolio-asset-item-loading-skeleton'

// Hooks
import usePricing from '../../../../../../common/hooks/pricing'

// Styled Components
import { Column, ScrollableColumn } from '../../../../../shared/style'
import {
  ButtonRow,
  DividerText,
  SubDivider,
  Spacer,
  FilterTokenRow
} from '../../style'

interface Props {
  userAssetList: UserAssetInfoType[]
  networks: BraveWallet.NetworkInfo[]
  renderToken: RenderTokenFunc
  hideAddButton?: boolean
  hideAssetFilter?: boolean
  hideAccountFilter?: boolean
  hideAutoDiscovery?: boolean
  enableScroll?: boolean
  maxListHeight?: string
  estimatedItemSize: number
}

export const TokenLists = ({
  userAssetList,
  networks,
  renderToken,
  hideAddButton,
  enableScroll,
  maxListHeight,
  hideAssetFilter,
  hideAccountFilter,
  hideAutoDiscovery
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

  const filteredAssetList = React.useMemo(() => {
    if (searchValue === '') {
      return visibleTokens.filter((asset) => asset.asset.visible)
    }
    return visibleTokens.filter((item) => {
      return (
        item.asset.name.toLowerCase() === searchValue.toLowerCase() ||
        item.asset.name.toLowerCase().startsWith(searchValue.toLowerCase()) ||
        item.asset.symbol.toLocaleLowerCase() === searchValue.toLowerCase() ||
        item.asset.symbol.toLowerCase().startsWith(searchValue.toLowerCase())
      )
    })
  }, [searchValue, visibleTokens])

  const [fungibleTokens, nonFungibleTokens] = React.useMemo(
    () => {
      let fungible = []
      let nonFungible = []
      for (const token of filteredAssetList) {
        if (token.asset.isErc721 || token.asset.isNft) {
          nonFungible.push(token)
        } else {
          fungible.push(token)
        }
      }
      return [fungible, nonFungible]
    },
    [filteredAssetList]
  )

  const assetFilterItemInfo = React.useMemo(() => {
    return AssetFilterOptions.find(item => item.id === selectedAssetFilter) ?? HighToLowAssetsFilterOption
  }, [selectedAssetFilter])

  const sortedFungibleTokensList: UserAssetInfoType[] = React.useMemo(() => {
    if (hideAssetFilter) {
      return fungibleTokens
    }
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
    computeFiatAmount,
    hideAssetFilter
  ])

  const listUi = React.useMemo(() => {
    return <>
      {sortedFungibleTokensList.map((token, index) => renderToken({ index, item: token, viewMode: 'list' }))}
      {!assetAutoDiscoveryCompleted && !hideAutoDiscovery &&
        <PortfolioAssetItemLoadingSkeleton />
      }
      {nonFungibleTokens.length !== 0 &&
        <>
          <Column fullWidth={true} alignItems='flex-start'>
            <Spacer />
            <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
            <SubDivider />
          </Column>
          {nonFungibleTokens.map((token, index) => renderToken({ index, item: token, viewMode: 'list' }))}
        </>
      }
    </>
  }, [
    sortedFungibleTokensList,
    renderToken,
    nonFungibleTokens,
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
      <FilterTokenRow>

        <Column flex={1} style={{ minWidth: '25%' }} alignItems='flex-start'>
          <SearchBar
            placeholder={getLocale('braveWalletSearchText')}
            action={onSearchValueChange}
            value={searchValue}
          />
        </Column>

        <NetworkFilterSelector networkListSubset={networks} />

        {!hideAssetFilter &&
          <AssetFilterSelector />
        }

        {!hideAccountFilter &&
          <AccountFilterSelector />
        }

      </FilterTokenRow>

      {enableScroll
        ? (
          <ScrollableColumn maxHeight={maxListHeight}>
            {listUi}
          </ScrollableColumn>
        )
        : listUi
      }

      {!hideAddButton &&
        <ButtonRow>
          <AddButton
            buttonType='secondary'
            onSubmit={showAddAssetsModal}
            text={getLocale('braveWalletAccountsEditVisibleAssets')}
          />
        </ButtonRow>
      }
    </>
  )
}
