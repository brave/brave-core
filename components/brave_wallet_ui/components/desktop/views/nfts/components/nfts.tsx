// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router-dom'
import { useDispatch } from 'react-redux'

// types
import {
  BraveWallet,
  WalletRoutes
} from '../../../../../constants/types'

// utils
import { getLocale } from '$web-common/locale'

// components
import SearchBar from '../../../../shared/search-bar'
import NetworkFilterSelector from '../../../network-filter-selector'

// styles
import {
  EmptyStateText,
  FilterTokenRow,
  NftGrid
} from './nfts.styles'
import { NFTGridViewItem } from '../../portfolio/components/nft-grid-view/nft-grid-view-item'
import { WalletPageActions } from '../../../../../page/actions'
import Amount from '../../../../../utils/amount'

interface Props {
  networks: BraveWallet.NetworkInfo[]
  nftList: BraveWallet.BlockchainToken[]
}

export const Nfts = (props: Props) => {
  const {
    networks,
    nftList
  } = props

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')

  // hooks
  const history = useHistory()
  const dispatch = useDispatch()

  // methods
  const onSearchValueChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setSearchValue(event.target.value)
  }, [])

  const onSelectAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    history.push(`${WalletRoutes.Portfolio}/${asset.contractAddress}/${asset.tokenId}`)
    // reset nft metadata
    dispatch(WalletPageActions.updateNFTMetadata(undefined))
  }, [dispatch])

  // memos
  const filteredNfts = React.useMemo(() => {
    if (searchValue === '') {
      return nftList
    }

    return nftList.filter((item) => {
      const tokenId = new Amount(item.tokenId).toNumber().toString()

      return (
        item.name.toLowerCase() === searchValue.toLowerCase() ||
        item.name.toLowerCase().includes(searchValue.toLowerCase()) ||
        item.symbol.toLocaleLowerCase() === searchValue.toLowerCase() ||
        item.symbol.toLowerCase().includes(searchValue.toLowerCase()) ||
        tokenId === searchValue.toLowerCase() ||
        tokenId.includes(searchValue.toLowerCase())
      )
    })
  }, [searchValue, nftList])

  const emptyStateMessage = React.useMemo(() => {
    return getLocale(searchValue === '' ? 'braveWalletNftsEmptyState' : 'braveWalletNftsEmptyStateSearch')
  }, [searchValue])

  return (
    <>
      <FilterTokenRow>
        <SearchBar
          placeholder={getLocale('braveWalletSearchText')}
          action={onSearchValueChange}
          value={searchValue}
        />
        <NetworkFilterSelector networkListSubset={networks} />
      </FilterTokenRow>
      {filteredNfts.length === 0
        ? <EmptyStateText>{emptyStateMessage}</EmptyStateText>
        : <NftGrid>
          {filteredNfts.map(nft => (
            <NFTGridViewItem
              key={`${nft.tokenId}-${nft.contractAddress}`}
              token={{ asset: nft, assetBalance: '' }}
              onSelectAsset={() => onSelectAsset(nft)}
            />
          ))}
        </NftGrid>
      }
    </>
  )
}
