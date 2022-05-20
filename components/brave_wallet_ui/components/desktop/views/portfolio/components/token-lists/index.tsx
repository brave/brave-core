// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Types
import {
  BraveWallet,
  UserAssetInfoType,
  DefaultCurrencies,
  WalletRoutes
} from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import { SearchBar } from '../../../../../shared'
import {
  PortfolioAssetItem,
  AddButton,
  NetworkFilterSelector
} from '../../../../'

// Styled Components
import {
  ButtonRow,
  DividerText,
  SubDivider,
  Spacer,
  FilterTokenRow
} from '../../style'

export interface Props {
  filteredAssetList: UserAssetInfoType[]
  tokenPrices: BraveWallet.AssetPrice[]
  defaultCurrencies: DefaultCurrencies
  userAssetList: UserAssetInfoType[]
  hideBalances: boolean
  networks: BraveWallet.NetworkInfo[]
  onSetFilteredAssetList: (filteredList: UserAssetInfoType[]) => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken | undefined) => () => void
}

const TokenLists = (props: Props) => {
  const {
    filteredAssetList,
    tokenPrices,
    defaultCurrencies,
    userAssetList,
    hideBalances,
    networks,
    onSelectAsset,
    onSetFilteredAssetList
  } = props

  // routing
  const history = useHistory()

  // memos
  const nonFungibleTokenList = React.useMemo(
    () => filteredAssetList.filter(({ asset }) => asset.isErc721),
    [filteredAssetList]
  )

  const fungibleTokenList = React.useMemo(
    () => filteredAssetList.filter(({ asset }) => !asset.isErc721),
    [filteredAssetList]
  )

  // methods
  // This filters a list of assets when the user types in search bar
  const onFilterAssets = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event.target.value
    if (search === '') {
      onSetFilteredAssetList(userAssetList)
    } else {
      const filteredList = userAssetList.filter((item) => {
        return (
          item.asset.name.toLowerCase() === search.toLowerCase() ||
          item.asset.name.toLowerCase().startsWith(search.toLowerCase()) ||
          item.asset.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          item.asset.symbol.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      onSetFilteredAssetList(filteredList)
    }
  }, [onSetFilteredAssetList, userAssetList])

  const showAddAssetsModal = React.useCallback(() => {
    history.push(WalletRoutes.AddAssetModal)
  }, [])

  // render
  return (
    <>
      <FilterTokenRow>
        <SearchBar placeholder={getLocale('braveWalletSearchText')} action={onFilterAssets} />
        <NetworkFilterSelector />
      </FilterTokenRow>
      {fungibleTokenList.map((item) =>
        <PortfolioAssetItem
          spotPrices={tokenPrices}
          defaultCurrencies={defaultCurrencies}
          action={onSelectAsset(item.asset)}
          key={`${item.asset.contractAddress}-${item.asset.symbol}-${item.asset.chainId}`}
          assetBalance={item.assetBalance}
          token={item.asset}
          hideBalances={hideBalances}
          networks={networks}
        />
      )}
      {nonFungibleTokenList.length !== 0 &&
        <>
          <Spacer />
          <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
          <SubDivider />
          {nonFungibleTokenList.map((item) =>
            <PortfolioAssetItem
              spotPrices={tokenPrices}
              defaultCurrencies={defaultCurrencies}
              action={onSelectAsset(item.asset)}
              key={`${item.asset.contractAddress}-${item.asset.tokenId}-${item.asset.chainId}`}
              assetBalance={item.assetBalance}
              token={item.asset}
              hideBalances={hideBalances}
              networks={networks}
            />
          )}
        </>
      }
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

export default TokenLists
