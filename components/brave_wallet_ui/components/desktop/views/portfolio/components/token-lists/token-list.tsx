// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useSelector } from 'react-redux'

// Types
import {
  BraveWallet,
  UserAssetInfoType,
  WalletRoutes,
  WalletState
} from '../../../../../../constants/types'
import { RenderTokenFunc } from './virtualized-tokens-list'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import SearchBar from '../../../../../shared/search-bar/index'
import AddButton from '../../../../add-button/index'
import NetworkFilterSelector from '../../../../network-filter-selector/index'
import { AccountFilterSelector } from '../../../../account-filter-selector/account-filter-selector'
import { AssetFilterSelector } from '../../../../asset-filter-selector/asset-filter-selector'
import { NFTGridView } from '../nft-grid-view/nft-grid-view'

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
  estimatedItemSize = 58
}: Props) => {
  // routing
  const history = useHistory()

  // redux
  const tokenSpotPrices = useSelector(({ wallet }: { wallet: WalletState }) => wallet.transactionSpotPrices)
  const selectedAssetFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedAssetFilter)

  // hooks
  const { computeFiatAmount } = usePricing(tokenSpotPrices)

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')

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
        if (token.asset.isErc721) {
          nonFungible.push(token)
        } else {
          fungible.push(token)
        }
      }
      return [fungible, nonFungible]
    },
    [filteredAssetList]
  )

  const sortedFungibleTokensList: UserAssetInfoType[] = React.useMemo(() => {
    if (
      selectedAssetFilter.id === 'highToLow' ||
      selectedAssetFilter.id === 'lowToHigh'
    ) {
      return [...fungibleTokens].sort(function (a, b) {
        const aBalance = a.assetBalance
        const bBalance = b.assetBalance
        const bFiatBalance = computeFiatAmount(bBalance, b.asset.symbol, b.asset.decimals)
        const aFiatBalance = computeFiatAmount(aBalance, a.asset.symbol, a.asset.decimals)
        return selectedAssetFilter.id === 'highToLow'
          ? bFiatBalance.toNumber() - aFiatBalance.toNumber()
          : aFiatBalance.toNumber() - bFiatBalance.toNumber()
      })
    }
    return fungibleTokens
  }, [fungibleTokens, selectedAssetFilter.id, computeFiatAmount])

  // computed
  // length of fungible tokens list is first NFT index
  const firstNftIndex = sortedFungibleTokensList.length || undefined

  const sortedFungibleTokensAndNftsList: UserAssetInfoType[] = React.useMemo(() => {
    return sortedFungibleTokensList.concat(nonFungibleTokens)
  }, [sortedFungibleTokensList, nonFungibleTokens])

  const listUi = React.useMemo(() => {
    return selectedAssetFilter.id !== 'nfts' ? (
        <>
          {sortedFungibleTokensList.map((token, index) => renderToken({ index, item: token, viewMode: 'list' }))}
          {nonFungibleTokens.length !== 0 &&
            <>
              <Spacer />
              <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
              <SubDivider />
              {nonFungibleTokens.map((token, index) => renderToken({ index, item: token, viewMode: 'list' }))}
            </>
          }
        </>
      ) : (
        <NFTGridView
          nonFungibleTokens={nonFungibleTokens}
          renderToken={(token, index) => renderToken({ index, item: token, viewMode: 'list' })}
        />
      )
  }, [
    firstNftIndex,
    selectedAssetFilter.id,
    sortedFungibleTokensAndNftsList,
    nonFungibleTokens,
    renderToken
  ])

  // effects
  React.useEffect(() => {
    // reset search field on list update
    if (userAssetList) {
      setSearchValue('')
    }
  }, [userAssetList])

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
