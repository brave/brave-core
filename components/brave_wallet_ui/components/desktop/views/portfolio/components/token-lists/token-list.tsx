// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import {
  useSelector
} from 'react-redux'

// Types
import {
  BraveWallet,
  UserAssetInfoType,
  WalletRoutes,
  WalletState
} from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import SearchBar from '../../../../../shared/search-bar/index'
import AddButton from '../../../../add-button/index'
import NetworkFilterSelector from '../../../../network-filter-selector/index'
import { AssetFilterSelector } from '../../../../asset-filter-selector/asset-filter-selector'
import { NFTGridView } from '../nft-grid-view/nft-grid-view'

// Hooks
import usePricing from '../../../../../../common/hooks/pricing'

// Styled Components
import {
  ButtonRow,
  DividerText,
  SubDivider,
  Spacer,
  FilterTokenRow
} from '../../style'

type ViewMode = 'list' | 'grid'

interface Props {
  userAssetList: UserAssetInfoType[]
  networks: BraveWallet.NetworkInfo[]
  renderToken: (item: UserAssetInfoType, viewMode: ViewMode) => JSX.Element
  hideAddButton?: boolean
  // enableScroll?: boolean
  // maxListHeight?: string
}

export const TokenLists = ({
  userAssetList,
  networks,
  renderToken,
  hideAddButton
  // enableScroll,
  // maxListHeight
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

  // memos
  const filteredAssetList = React.useMemo(() => {
    if (searchValue === '') {
      return userAssetList
    }
    return userAssetList.filter((item) => {
      return (
        item.asset.name.toLowerCase() === searchValue.toLowerCase() ||
        item.asset.name.toLowerCase().startsWith(searchValue.toLowerCase()) ||
        item.asset.symbol.toLocaleLowerCase() === searchValue.toLowerCase() ||
        item.asset.symbol.toLowerCase().startsWith(searchValue.toLowerCase())
      )
    })
  }, [searchValue, userAssetList])

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
      return fungibleTokens.sort(function (a, b) {
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
  }, [fungibleTokens, selectedAssetFilter, computeFiatAmount])

  // const listUi = React.useMemo(() => {
  //   return <>
  //     {fungibleTokens.map(renderToken)}

  //     {nonFungibleTokens.length !== 0 &&
  //       <>
  //         <Spacer />
  //         <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
  //         <SubDivider />
  //         {nonFungibleTokens.map(renderToken)}
  //       </>
  //     }
  //   </>
  // }, [fungibleTokens, nonFungibleTokens, renderToken])

  // methods

  // This filters a list of assets when the user types in search bar
  const onSearchValueChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setSearchValue(event.target.value)
  }, [])

  const showAddAssetsModal = React.useCallback(() => {
    history.push(WalletRoutes.AddAssetModal)
  }, [])

  const renderTokenInViewMode = React.useCallback((mode: ViewMode) => {
    return (token: UserAssetInfoType) => {
      return renderToken(token, mode)
    }
  }, [renderToken])

  // render
  return (
    <>
      <FilterTokenRow>
        <SearchBar
          placeholder={getLocale('braveWalletsearchValue')}
          action={onSearchValueChange}
          value={searchValue}
        />
        <NetworkFilterSelector networkListSubset={networks} />
        <AssetFilterSelector />
      </FilterTokenRow>

      {/* {enableScroll
        ? <ScrollableColumn maxHeight={maxListHeight}>
            {listUi}
          </ScrollableColumn>
        : listUi
      } */}

      {selectedAssetFilter.id !== 'nfts' ? (
        <>
          {sortedFungibleTokensList.map(renderTokenInViewMode('list'))}
          {nonFungibleTokens.length !== 0 &&
            <>
              <Spacer />
              <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
              <SubDivider />
              {nonFungibleTokens.map(renderTokenInViewMode('list'))}
            </>
          }
        </>
      ) : (
        <NFTGridView
          nonFungibleTokens={nonFungibleTokens}
          renderToken={renderTokenInViewMode('grid')}
        />
      )}

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
