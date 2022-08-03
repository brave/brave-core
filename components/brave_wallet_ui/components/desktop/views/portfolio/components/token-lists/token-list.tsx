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
  DefaultCurrencies,
  WalletRoutes,
  WalletState
} from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import { SearchBar } from '../../../../../shared'
import {
  PortfolioAssetItem,
  AddButton,
  NetworkFilterSelector,
  AssetFilterSelector
} from '../../../..'
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

interface Props {
  tokenPrices: BraveWallet.AssetPrice[]
  defaultCurrencies: DefaultCurrencies
  userAssetList: UserAssetInfoType[]
  hideBalances: boolean
  networks: BraveWallet.NetworkInfo[]
  onSelectAsset: (asset: BraveWallet.BlockchainToken | undefined) => void
}

export const TokenLists = (props: Props) => {
  const {
    tokenPrices,
    defaultCurrencies,
    userAssetList,
    hideBalances,
    networks,
    onSelectAsset
  } = props

  // routing
  const history = useHistory()

  // redux
  const selectedAssetFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedAssetFilter)

  // hooks
  const { computeFiatAmount } = usePricing(tokenPrices)

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')

  // memos
  const filteredAssetList: UserAssetInfoType[] = React.useMemo(() => {
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

  const nonFungibleTokens: UserAssetInfoType[] = React.useMemo(
    () =>
      filteredAssetList.filter((token) => token.asset.isErc721
      ),
    [filteredAssetList])

  const sortedAssetList: UserAssetInfoType[] = React.useMemo(() => {
    const fungibleTokens = filteredAssetList.filter((asset) => !asset.asset.isErc721)
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
  }, [filteredAssetList, selectedAssetFilter, computeFiatAmount])

  // methods

  // This filters a list of assets when the user types in search bar
  const onFilterAssets = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setSearchValue(event.target.value)
  }, [])

  const showAddAssetsModal = React.useCallback(() => {
    history.push(WalletRoutes.AddAssetModal)
  }, [])

  // render
  return (
    <>
      <FilterTokenRow>
        <SearchBar placeholder={getLocale('braveWalletSearchText')} action={onFilterAssets} />
        <NetworkFilterSelector />
        <AssetFilterSelector />
      </FilterTokenRow>
      {selectedAssetFilter.id !== 'nfts' ? (
        <>
          {sortedAssetList.map((item) =>
            <PortfolioAssetItem
              spotPrices={tokenPrices}
              defaultCurrencies={defaultCurrencies}
              action={() => onSelectAsset(item.asset)}
              key={`${item.asset.contractAddress}-${item.asset.symbol}-${item.asset.chainId}`}
              assetBalance={item.assetBalance}
              token={item.asset}
              hideBalances={hideBalances}
              networks={networks}
            />
          )}
          {nonFungibleTokens.length !== 0 &&
            <>
              <Spacer />
              <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
              <SubDivider />
              {nonFungibleTokens.map((item) =>
                <PortfolioAssetItem
                  spotPrices={tokenPrices}
                  defaultCurrencies={defaultCurrencies}
                  action={() => onSelectAsset(item.asset)}
                  key={`${item.asset.contractAddress}-${item.asset.tokenId}-${item.asset.chainId}`}
                  assetBalance={item.assetBalance}
                  token={item.asset}
                  hideBalances={hideBalances}
                  networks={networks}
                />
              )}
            </>
          }
        </>
      ) : (
        <NFTGridView
          nonFungibleTokens={nonFungibleTokens}
          onSelectAsset={onSelectAsset}
        />
      )}
      <ButtonRow>
        <AddButton
          buttonType='secondary'
          onSubmit={showAddAssetsModal}
          text={getLocale('braveWalletAccountsEditVisibleAssets')}
        />
      </ButtonRow>
    </>
  )
}
