// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router-dom'
import { useDispatch } from 'react-redux'

// types
import {
  BraveWallet,
  WalletRoutes
} from '../../../../../constants/types'

// hooks
import { useNftPin } from '../../../../../common/hooks/nft-pin'

// selectors
import { useSafeWalletSelector } from '../../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../../common/selectors'

// actions
import { WalletPageActions } from '../../../../../page/actions'

// utils
import { getLocale } from '$web-common/locale'
import Amount from '../../../../../utils/amount'

// components
import SearchBar from '../../../../shared/search-bar'
import NetworkFilterSelector from '../../../network-filter-selector'
import { NFTGridViewItem } from '../../portfolio/components/nft-grid-view/nft-grid-view-item'

// styles
import {
  StyledWrapper,
  FilterTokenRow,
  IpfsButton,
  IpfsIcon,
  NftGrid,
  AddIcon,
  AddButton
} from './nfts.styles'
import { AddOrEditNftModal } from '../../../popup-modals/add-edit-nft-modal/add-edit-nft-modal'
import { NftsEmptyState } from './nfts-empty-state/nfts-empty-state'

interface Props {
  networks: BraveWallet.NetworkInfo[]
  nftList: BraveWallet.BlockchainToken[]
  onToggleShowIpfsBanner: () => void
}

export const Nfts = (props: Props) => {
  const {
    networks,
    nftList,
    onToggleShowIpfsBanner
  } = props

  const isNftPinningFeatureEnabled = useSafeWalletSelector(WalletSelectors.isNftPinningFeatureEnabled)

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showAddNftModal, setShowAddNftModal] = React.useState<boolean>(false)

  // hooks
  const history = useHistory()
  const dispatch = useDispatch()
  const { nonFungibleTokens } = useNftPin()

  // methods
  const onSearchValueChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setSearchValue(event.target.value)
  }, [])

  const onSelectAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    history.push(`${WalletRoutes.Portfolio}/${asset.contractAddress}/${asset.tokenId}`)
    // reset nft metadata
    dispatch(WalletPageActions.updateNFTMetadata(undefined))
  }, [dispatch])

  const onClickIpfsButton = React.useCallback(() => {
    onToggleShowIpfsBanner()
  }, [onToggleShowIpfsBanner])

  const toggleShowAddNftModal = React.useCallback(() => {
    setShowAddNftModal(value => !value)
  }, [])

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

  const sortedNfts = React.useMemo(() => {
    return filteredNfts.sort((a, b) => a.name.localeCompare(b.name))
  }, [filteredNfts])

  return (
    <StyledWrapper>
      <FilterTokenRow>
        <SearchBar
          placeholder={getLocale('braveWalletSearchText')}
          action={onSearchValueChange}
          value={searchValue}
        />
        <NetworkFilterSelector networkListSubset={networks} />
        {isNftPinningFeatureEnabled && nonFungibleTokens.length > 0 &&
          <IpfsButton onClick={onClickIpfsButton}>
            <IpfsIcon />
          </IpfsButton>
        }
        <AddButton onClick={toggleShowAddNftModal}>
          <AddIcon />
        </AddButton>
      </FilterTokenRow>
      {sortedNfts.length === 0
        ? <NftsEmptyState onImportNft={toggleShowAddNftModal} />
        : <NftGrid>
          {sortedNfts.map(nft => (
            <NFTGridViewItem
              key={`${nft.tokenId}-${nft.contractAddress}`}
              token={nft}
              onSelectAsset={() => onSelectAsset(nft)}
            />
          ))}
        </NftGrid>
      }
      {showAddNftModal &&
        <AddOrEditNftModal
          onClose={toggleShowAddNftModal}
          onHideForm={toggleShowAddNftModal}
        />
      }
    </StyledWrapper>
  )
}
