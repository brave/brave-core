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
import { useSafeWalletSelector, useUnsafeWalletSelector } from '../../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../../common/selectors'

// actions
import { WalletActions } from '../../../../../common/actions'
import { WalletPageActions } from '../../../../../page/actions'

// utils
import { getLocale } from '$web-common/locale'
import Amount from '../../../../../utils/amount'
import {
  LOCAL_STORAGE_KEYS
} from '../../../../../common/constants/local-storage-keys'
import {
  useGetNftDiscoveryEnabledStatusQuery,
  useSetNftDiscoveryEnabledMutation
} from '../../../../../common/slices/api.slice'

// components
import SearchBar from '../../../../shared/search-bar'
import NetworkFilterSelector from '../../../network-filter-selector'
import { NFTGridViewItem } from '../../portfolio/components/nft-grid-view/nft-grid-view-item'
import { EnableNftDiscoveryModal } from '../../../popup-modals/enable-nft-discovery-modal/enable-nft-discovery-modal'
import { AutoDiscoveryEmptyState } from './auto-discovery-empty-state/auto-discovery-empty-state'
import { TabOption, Tabs } from '../../../../shared/tabs/tabs'

// styles
import {
  FilterTokenRow,
  IpfsButton,
  IpfsIcon,
  NftGrid,
  AddIcon,
  AddButton
} from './nfts.styles'
import { Row, ScrollableColumn } from '../../../../shared/style'
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

  // redux
  const isNftPinningFeatureEnabled = useSafeWalletSelector(WalletSelectors.isNftPinningFeatureEnabled)
  const hiddenNfts = useUnsafeWalletSelector(WalletSelectors.removedNonFungibleTokens)

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showAddNftModal, setShowAddNftModal] = React.useState<boolean>(false)
  const [showNftDiscoveryModal, setShowNftDiscoveryModal] = React.useState<boolean>(
    localStorage.getItem(LOCAL_STORAGE_KEYS.IS_ENABLE_NFT_AUTO_DISCOVERY_MODAL_HIDDEN) === null
  )
  const [selectedTab, setSelectedTab] = React.useState<string>('nfts')

  // hooks
  const history = useHistory()
  const dispatch = useDispatch()
  const { nonFungibleTokens } = useNftPin()

  // queries
  const { data: isNftAutoDiscoveryEnabled } = useGetNftDiscoveryEnabledStatusQuery()

  // mutations
  const [setNftDiscovery] = useSetNftDiscoveryEnabledMutation()

  // methods
  const onSearchValueChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setSearchValue(event.target.value)
  }, [])

  const onSelectAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    history.push(
      `${WalletRoutes.PortfolioNFTs //
      }/${asset.chainId //
      }/${asset.contractAddress //
      }/${asset.tokenId}`
    )
    // reset nft metadata
    dispatch(WalletPageActions.updateNFTMetadata(undefined))
  }, [dispatch])

  const onClickIpfsButton = React.useCallback(() => {
    onToggleShowIpfsBanner()
  }, [onToggleShowIpfsBanner])

  const toggleShowAddNftModal = React.useCallback(() => {
    setShowAddNftModal(value => !value)
  }, [])

  const hideNftDiscoveryModal = React.useCallback(() => {
    setShowNftDiscoveryModal(current => {
      localStorage.setItem(LOCAL_STORAGE_KEYS.IS_ENABLE_NFT_AUTO_DISCOVERY_MODAL_HIDDEN, 'true')
      return !current
    })
  }, [])

  const onConfirmNftAutoDiscovery = React.useCallback(async () => {
    await setNftDiscovery(true)
    hideNftDiscoveryModal()
  }, [hideNftDiscoveryModal, setNftDiscovery])

  const onRefresh = React.useCallback(() => {
    dispatch(WalletActions.refreshNetworksAndTokens({}))
  }, [])

  const onSelectTab = React.useCallback((selectedTab: TabOption) => {
    setSelectedTab(selectedTab.id)
  }, [])

  const searchNfts = React.useCallback((item: BraveWallet.BlockchainToken) => {
    const tokenId = new Amount(item.tokenId).toNumber().toString()

    return (
      item.name.toLowerCase() === searchValue.toLowerCase() ||
      item.name.toLowerCase().includes(searchValue.toLowerCase()) ||
      item.symbol.toLocaleLowerCase() === searchValue.toLowerCase() ||
      item.symbol.toLowerCase().includes(searchValue.toLowerCase()) ||
      tokenId === searchValue.toLowerCase() ||
      tokenId.includes(searchValue.toLowerCase())
    )
  }, [searchValue])


  // memos
  const [sortedNfts, sortedHiddenNfts] = React.useMemo(() => {
    const compareFn = (a: BraveWallet.BlockchainToken, b: BraveWallet.BlockchainToken) => a.name.localeCompare(b.name)

    if (searchValue === '') {
      return [
        nftList.slice().sort(compareFn),
        hiddenNfts.slice().sort(compareFn)
      ]
    }

    return [
      nftList.filter(searchNfts).sort(compareFn),
      hiddenNfts.filter(searchNfts).sort(compareFn)
    ]
  }, [searchValue, nftList, hiddenNfts, searchNfts])

  const tabOptions = React.useMemo(() => {
    const tabOptions: TabOption[] = [
      {
        id: 'nfts',
        label: getLocale('braveNftsTab'),
        labelSummary: nftList.length || undefined
      },
      {
        id: 'hidden',
        label: getLocale('braveNftsTabHidden'),
        labelSummary: hiddenNfts.length || undefined
      }
    ]

    return tabOptions
  }, [nftList, hiddenNfts])


  const renderedList = React.useMemo(() => {
    return selectedTab === 'nfts' ? sortedNfts : sortedHiddenNfts
  }, [selectedTab, sortedNfts, sortedHiddenNfts])

  return (
    <>
      <FilterTokenRow>
        <SearchBar
          placeholder={getLocale('braveWalletSearchText')}
          action={onSearchValueChange}
          value={searchValue}
        />
        <NetworkFilterSelector networkListSubset={networks} />
        {isNftPinningFeatureEnabled && nonFungibleTokens.length > 0 && (
          <IpfsButton onClick={onClickIpfsButton}>
            <IpfsIcon />
          </IpfsButton>
        )}
        <AddButton onClick={toggleShowAddNftModal}>
          <AddIcon />
        </AddButton>
      </FilterTokenRow>
      <Row justifyContent='flex-start' padding='23px 0 23px 23px'>
        <Tabs
          options={tabOptions}
          onSelect={onSelectTab}
        />
      </Row>
      <ScrollableColumn padding='10px 20px 20px 20px'>
        {(nftList.length === 0 && hiddenNfts.length === 0) ? (
          isNftAutoDiscoveryEnabled ? (
            <AutoDiscoveryEmptyState
              onImportNft={toggleShowAddNftModal}
              onRefresh={onRefresh}
            />
          ) : (
            <NftsEmptyState onImportNft={toggleShowAddNftModal} />
          )
        ) : (
          <NftGrid>
            {renderedList.map((nft) => (
              <NFTGridViewItem
                isHidden={selectedTab === 'hidden'}
                key={`${nft.tokenId}-${nft.contractAddress}`}
                token={nft}
                onSelectAsset={() => onSelectAsset(nft)}
              />
            ))}
          </NftGrid>
        )}
      </ScrollableColumn>
      {showAddNftModal && (
        <AddOrEditNftModal
          onClose={toggleShowAddNftModal}
          onHideForm={toggleShowAddNftModal}
        />
      )}
      {!isNftAutoDiscoveryEnabled && showNftDiscoveryModal && (
        <EnableNftDiscoveryModal
          onConfirm={onConfirmNftAutoDiscovery}
          onCancel={hideNftDiscoveryModal}
        />
      )}
    </>
  )
}
