// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useParams } from 'react-router-dom'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query'

// types
import { BraveWallet } from '../../../../../constants/types'

// hooks
import { useAccountsQuery } from '../../../../../common/slices/api.slice.extra'
import {
  useBalancesFetcher //
} from '../../../../../common/hooks/use-balances-fetcher'
import {
  useSyncedLocalStorage //
} from '../../../../../common/hooks/use_local_storage'

// selectors
import {
  useSafeUISelector,
  useSafeWalletSelector
} from '../../../../../common/hooks/use-safe-selector'
import { UISelectors, WalletSelectors } from '../../../../../common/selectors'

// actions
import { WalletActions } from '../../../../../common/actions'
import { WalletPageActions } from '../../../../../page/actions'

// utils
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../../common/constants/local-storage-keys'
import {
  useGetNftAssetIdsByCollectionRegistryQuery,
  useGetNftDiscoveryEnabledStatusQuery,
  useGetSimpleHashSpamNftsQuery,
  useGetUserTokensRegistryQuery,
  useSetNftDiscoveryEnabledMutation
} from '../../../../../common/slices/api.slice'
import {
  compareTokensByName,
  filterTokensByNetworks,
  getAllSpamNftsAndIds,
  getAssetIdKey,
  getTokenCollectionName,
  getTokensWithBalanceForAccounts,
  groupSpamAndNonSpamNfts,
  isTokenWatchOnly
} from '../../../../../utils/asset-utils'
import { useQuery } from '../../../../../common/hooks/use-query'
import {
  makePortfolioAssetRoute,
  makePortfolioNftCollectionRoute
} from '../../../../../utils/routes-utils'
import {
  selectAllVisibleUserNFTsFromQueryResult,
  selectHiddenNftsFromQueryResult //
} from '../../../../../common/slices/entities/blockchain-token.entity'
import {
  getLastPageNumber,
  getListPageItems
} from '../../../../../utils/pagination_utils'

// components
import {
  NFTGridViewItem //
} from '../../portfolio/components/nft-grid-view/nft-grid-view-item'
import {
  EnableNftDiscoveryModal //
} from '../../../popup-modals/enable-nft-discovery-modal/enable-nft-discovery-modal'
import {
  AutoDiscoveryEmptyState //
} from './auto-discovery-empty-state/auto-discovery-empty-state'
import {
  NftGridViewItemSkeleton //
} from '../../portfolio/components/nft-grid-view/nft-grid-view-item-skeleton'
import { Pagination } from '../../../../shared/pagination/pagination'
import {
  WalletPageWrapper //
} from '../../../wallet-page-wrapper/wallet-page-wrapper'
import NftAssetHeader from '../../../card-headers/nft-asset-header'
import { NftsEmptyState } from './nfts-empty-state/nfts-empty-state'
import {
  AddOrEditNftModal //
} from '../../../popup-modals/add-edit-nft-modal/add-edit-nft-modal'

// styles
import { NFTListWrapper, NftGrid } from './nfts.styles'
import { Column, Row } from '../../../../shared/style'
import { ContentWrapper } from '../../portfolio/style'

interface Params {
  collectionName: string
}

interface Props {
  accounts: BraveWallet.AccountInfo[]
  networks: BraveWallet.NetworkInfo[]
}

const LIST_PAGE_ITEM_COUNT = 15

const scrollOptions: ScrollIntoViewOptions = { block: 'start' }

const emptyTokenIdsList: string[] = []

export const NftCollection = ({ networks, accounts }: Props) => {
  // routing
  const history = useHistory()

  const { collectionName } = useParams<Params>()
  const urlSearchParams = useQuery()
  const currentPageNumber = Number(urlSearchParams.get('page')) || 1

  // refs
  const listScrollContainerRef = React.useRef<HTMLDivElement>(null)

  // redux
  const dispatch = useDispatch()
  const assetAutoDiscoveryCompleted = useSafeWalletSelector(
    WalletSelectors.assetAutoDiscoveryCompleted
  )
  const isRefreshingTokens = useSafeWalletSelector(
    WalletSelectors.isRefreshingNetworksAndTokens
  )
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // local-storage
  const [hideUnownedNfts] = useSyncedLocalStorage<boolean>(
    LOCAL_STORAGE_KEYS.HIDE_UNOWNED_NFTS,
    false
  )

  // state
  const [showAddNftModal, setShowAddNftModal] = React.useState<boolean>(false)
  const [showNftDiscoveryModal, setShowNftDiscoveryModal] =
    React.useState<boolean>(
      localStorage.getItem(
        LOCAL_STORAGE_KEYS.IS_ENABLE_NFT_AUTO_DISCOVERY_MODAL_HIDDEN
      ) === null
    )

  // queries
  const { data: isNftAutoDiscoveryEnabled } =
    useGetNftDiscoveryEnabledStatusQuery()
  const { accounts: allAccounts, isLoading: isLoadingAccounts } =
    useAccountsQuery()
  const { data: simpleHashSpamNfts, isFetching: isLoadingSimpleHashNfts } =
    useGetSimpleHashSpamNftsQuery(
      accounts.length
        ? {
            accounts
          }
        : skipToken
    )
  const { userTokensRegistry, hiddenNfts, visibleNfts, isFetchingUserTokens } =
    useGetUserTokensRegistryQuery(undefined, {
      selectFromResult: (result) => ({
        isFetchingUserTokens: result.isFetching,
        userTokensRegistry: result.data,
        visibleNfts: selectAllVisibleUserNFTsFromQueryResult(result),
        hiddenNfts: selectHiddenNftsFromQueryResult(result)
      })
    })

  const { data: tokenBalancesRegistry, isLoading: isLoadingTokenBalances } =
    // will fetch balances for all accounts so we can filter NFTs by accounts
    useBalancesFetcher(
      isFetchingUserTokens || networks.length === 0 || allAccounts.length === 0
        ? skipToken
        : {
            accounts: allAccounts,
            networks
          }
    )

  const hiddenNftsIds =
    userTokensRegistry?.nonFungibleHiddenTokenIds ?? emptyTokenIdsList
  const userNonSpamNftIds =
    userTokensRegistry?.nonSpamTokenIds ?? emptyTokenIdsList

  const shouldFetchSpamNftBalances =
    !isLoadingSimpleHashNfts &&
    hideUnownedNfts &&
    accounts.length > 0 &&
    networks.length > 0

  const { data: spamTokenBalancesRegistry, isLoading: isLoadingSpamBalances } =
    useBalancesFetcher(
      shouldFetchSpamNftBalances
        ? {
            accounts,
            networks,
            isSpamRegistry: true
          }
        : skipToken
    )

  // mutations
  const [setNftDiscovery] = useSetNftDiscoveryEnabledMutation()

  // memos & computed
  const { visibleUserNonSpamNfts, visibleUserMarkedSpamNfts } =
    React.useMemo(() => {
      return groupSpamAndNonSpamNfts(visibleNfts)
    }, [visibleNfts])

  const [allSpamNfts, allSpamNftsIds] = React.useMemo(() => {
    return getAllSpamNftsAndIds(
      userNonSpamNftIds,
      hiddenNftsIds,
      userTokensRegistry?.deletedTokenIds || [],
      simpleHashSpamNfts || [],
      visibleUserMarkedSpamNfts
    )
  }, [
    visibleUserMarkedSpamNfts,
    simpleHashSpamNfts,
    hiddenNftsIds,
    userNonSpamNftIds,
    userTokensRegistry
  ])

  const hiddenAndSpamNfts = React.useMemo(() => {
    return hiddenNfts.concat(allSpamNfts)
  }, [allSpamNfts, hiddenNfts])

  const allNfts = React.useMemo(() => {
    return visibleUserNonSpamNfts.concat(hiddenAndSpamNfts)
  }, [hiddenAndSpamNfts, visibleUserNonSpamNfts])

  const sortedSelectedNftList = React.useMemo(() => {
    return allNfts.slice().sort(compareTokensByName)
  }, [allNfts])

  // Filters the user's tokens based on the users
  // filteredOutPortfolioNetworkKeys pref and visible networks.
  const sortedSelectedNftListForChains = React.useMemo(() => {
    return filterTokensByNetworks(sortedSelectedNftList, networks)
  }, [sortedSelectedNftList, networks])

  // apply accounts filter to selected nfts list
  const sortedSelectedNftListForChainsAndAccounts = React.useMemo(() => {
    return getTokensWithBalanceForAccounts(
      sortedSelectedNftListForChains,
      accounts,
      allAccounts,
      tokenBalancesRegistry,
      spamTokenBalancesRegistry,
      hideUnownedNfts
    )
  }, [
    accounts,
    allAccounts,
    hideUnownedNfts,
    sortedSelectedNftListForChains,
    spamTokenBalancesRegistry,
    tokenBalancesRegistry
  ])

  // differed queries
  const { data: nftAssetIdsByCollectionRegistryInfo } =
    useGetNftAssetIdsByCollectionRegistryQuery(
      sortedSelectedNftListForChainsAndAccounts.length
        ? sortedSelectedNftListForChainsAndAccounts
        : skipToken
    )
  const nftAssetIdsByCollectionRegistry =
    nftAssetIdsByCollectionRegistryInfo?.registry

  // memos
  const collectionNames = React.useMemo(() => {
    return Object.keys(nftAssetIdsByCollectionRegistry || {})
  }, [nftAssetIdsByCollectionRegistry])

  const nftListForCollection = React.useMemo(() => {
    return sortedSelectedNftListForChainsAndAccounts.filter((token) => {
      return (
        getTokenCollectionName(
          collectionNames,
          nftAssetIdsByCollectionRegistry,
          token
        ) === collectionName
      )
    })
  }, [
    collectionName,
    collectionNames,
    nftAssetIdsByCollectionRegistry,
    sortedSelectedNftListForChainsAndAccounts
  ])

  const lastPageNumber = getLastPageNumber(
    nftListForCollection,
    LIST_PAGE_ITEM_COUNT
  )

  const renderedListPage = React.useMemo(() => {
    return getListPageItems(
      nftListForCollection,
      currentPageNumber,
      LIST_PAGE_ITEM_COUNT
    )
  }, [nftListForCollection, currentPageNumber])

  const isLoadingAssets =
    isLoadingTokenBalances ||
    isLoadingSimpleHashNfts ||
    isFetchingUserTokens ||
    isLoadingAccounts ||
    !assetAutoDiscoveryCompleted ||
    !allSpamNfts ||
    (shouldFetchSpamNftBalances && isLoadingSpamBalances)

  // methods
  const onSelectAsset = React.useCallback(
    (asset: BraveWallet.BlockchainToken) => {
      history.push(makePortfolioAssetRoute(true, getAssetIdKey(asset)))
      // reset nft metadata
      dispatch(WalletPageActions.updateNFTMetadata(undefined))
    },
    [dispatch, history]
  )

  const toggleShowAddNftModal = React.useCallback(() => {
    setShowAddNftModal((value) => !value)
  }, [])

  const hideNftDiscoveryModal = React.useCallback(() => {
    setShowNftDiscoveryModal((current) => {
      localStorage.setItem(
        LOCAL_STORAGE_KEYS.IS_ENABLE_NFT_AUTO_DISCOVERY_MODAL_HIDDEN,
        'true'
      )
      return !current
    })
  }, [])

  const onConfirmNftAutoDiscovery = React.useCallback(async () => {
    await setNftDiscovery(true)
    hideNftDiscoveryModal()
  }, [hideNftDiscoveryModal, setNftDiscovery])

  const onRefresh = React.useCallback(() => {
    dispatch(WalletActions.refreshNetworksAndTokens())
  }, [dispatch])

  const navigateToPage = React.useCallback(
    (pageNumber: number) => {
      history.push(makePortfolioNftCollectionRoute(collectionName, pageNumber))
      listScrollContainerRef.current?.scrollIntoView(scrollOptions)
    },
    [collectionName, history]
  )

  // render
  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      noCardPadding={false}
      hideDivider={false}
      cardHeader={
        <NftAssetHeader
          onBack={history.goBack}
          assetName={collectionName || ''}
        />
      }
    >
      <ContentWrapper
        fullWidth={true}
        fullHeight={isPanel}
        justifyContent='flex-start'
        isPanel={isPanel}
      >
        <NFTListWrapper
          ref={listScrollContainerRef}
          fullHeight
        >
          {visibleNfts.length === 0 &&
          userTokensRegistry?.hiddenTokenIds.length === 0 ? (
            isNftAutoDiscoveryEnabled ? (
              <AutoDiscoveryEmptyState
                isRefreshingTokens={isRefreshingTokens}
                onImportNft={toggleShowAddNftModal}
                onRefresh={onRefresh}
              />
            ) : (
              <NftsEmptyState onImportNft={toggleShowAddNftModal} />
            )
          ) : (
            <>
              <NftGrid padding='0px'>
                {renderedListPage.map((nft) => {
                  const assetId = getAssetIdKey(nft)
                  const isSpam = allSpamNftsIds.includes(assetId)
                  const isHidden =
                    isSpam ||
                    Boolean(
                      userTokensRegistry?.nonFungibleHiddenTokenIds.includes(
                        assetId
                      )
                    )
                  return (
                    <NFTGridViewItem
                      key={assetId}
                      token={nft}
                      onSelectAsset={onSelectAsset}
                      isTokenHidden={isHidden}
                      isTokenSpam={isSpam}
                      isWatchOnly={isTokenWatchOnly(
                        nft,
                        allAccounts,
                        tokenBalancesRegistry,
                        spamTokenBalancesRegistry
                      )}
                    />
                  )
                })}
                {isLoadingAssets && <NftGridViewItemSkeleton />}
              </NftGrid>

              <Row
                width='100%'
                margin={'24px 0px 0px 0px'}
              >
                <Column width='50%'>
                  <Pagination
                    onSelectPageNumber={navigateToPage}
                    currentPageNumber={currentPageNumber}
                    lastPageNumber={lastPageNumber}
                  />
                </Column>
              </Row>
            </>
          )}
        </NFTListWrapper>
        {showAddNftModal && (
          <AddOrEditNftModal
            onClose={toggleShowAddNftModal}
            onHideForm={toggleShowAddNftModal}
          />
        )}
        {isNftAutoDiscoveryEnabled !== undefined &&
          !isNftAutoDiscoveryEnabled &&
          showNftDiscoveryModal && (
            <EnableNftDiscoveryModal
              onConfirm={onConfirmNftAutoDiscovery}
              onCancel={hideNftDiscoveryModal}
            />
          )}
      </ContentWrapper>
    </WalletPageWrapper>
  )
}
