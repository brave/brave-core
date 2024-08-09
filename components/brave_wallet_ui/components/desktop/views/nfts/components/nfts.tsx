// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router-dom'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query'

// types
import {
  BraveWallet,
  NftDropdownOptionId
} from '../../../../../constants/types'

// hooks
import { useAccountsQuery } from '../../../../../common/slices/api.slice.extra'
import {
  useBalancesFetcher //
} from '../../../../../common/hooks/use-balances-fetcher'
import {
  useLocalStorage,
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
import getAPIProxy from '../../../../../common/async/bridge'
import { getLocale } from '$web-common/locale'
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
  isTokenWatchOnly,
  searchNftCollectionsAndGetTotalNftsFound,
  searchNfts
} from '../../../../../utils/asset-utils'
import { useQuery } from '../../../../../common/hooks/use-query'
import {
  makePortfolioAssetRoute,
  makePortfolioNftCollectionRoute,
  makePortfolioNftsRoute
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
import SearchBar from '../../../../shared/search-bar'
import { NFTGridViewItem } from '../../portfolio/components/nft-grid-view/nft-grid-view-item'
import { EnableNftDiscoveryModal } from '../../../popup-modals/enable-nft-discovery-modal/enable-nft-discovery-modal'
import { AutoDiscoveryEmptyState } from './auto-discovery-empty-state/auto-discovery-empty-state'
import {
  NftGridViewItemSkeleton //
} from '../../portfolio/components/nft-grid-view/nft-grid-view-item-skeleton'
import { Pagination } from '../../../../shared/pagination/pagination'
import {
  NftDropdown,
  NftDropdownOption
} from './nft-group-selector/nft-group-selector'
import {
  NftCollectionGridViewItem //
} from '../../portfolio/components/nft-grid-view/nft-collection-grid-view-item'

// styles
import { NFTListWrapper, NftGrid } from './nfts.styles'
import { Column, Row } from '../../../../shared/style'
import { AddOrEditNftModal } from '../../../popup-modals/add-edit-nft-modal/add-edit-nft-modal'
import { NftsEmptyState } from './nfts-empty-state/nfts-empty-state'
import {
  ButtonIcon,
  PortfolioActionButton,
  SearchBarWrapper,
  ControlBarWrapper,
  ContentWrapper
} from '../../portfolio/style'

interface Props {
  onShowPortfolioSettings?: () => void
  accounts: BraveWallet.AccountInfo[]
  networks: BraveWallet.NetworkInfo[]
}

const LIST_PAGE_ITEM_COUNT = 15

const scrollOptions: ScrollIntoViewOptions = { block: 'start' }

const emptyTokenIdsList: string[] = []

export const Nfts = ({
  networks,
  accounts,
  onShowPortfolioSettings
}: Props) => {
  // routing
  const history = useHistory()
  const urlSearchParams = useQuery()
  const tab = urlSearchParams.get('tab')
  const currentPageNumber = Number(urlSearchParams.get('page')) || 1
  const selectedTab: NftDropdownOptionId =
    tab === 'collected' || tab === 'hidden' ? tab : 'collected'

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
  const [groupNftsByCollection] = useLocalStorage<boolean>(
    LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_NFTS_BY_COLLECTION,
    false
  )

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showAddNftModal, setShowAddNftModal] = React.useState<boolean>(false)
  const [showNftDiscoveryModal, setShowNftDiscoveryModal] =
    React.useState<boolean>(
      localStorage.getItem(
        LOCAL_STORAGE_KEYS.IS_ENABLE_NFT_AUTO_DISCOVERY_MODAL_HIDDEN
      ) === null
    )
  const [showSearchBar, setShowSearchBar] = React.useState<boolean>(false)

  // queries
  const { data: isNftAutoDiscoveryEnabled } =
    useGetNftDiscoveryEnabledStatusQuery()
  const { data: simpleHashSpamNfts = [], isFetching: isLoadingSpamNfts } =
    useGetSimpleHashSpamNftsQuery(
      selectedTab === 'collected' || !accounts.length ? skipToken : { accounts }
    )
  const { accounts: allAccounts } = useAccountsQuery()
  const { userTokensRegistry, hiddenNfts, visibleNfts, isFetchingTokens } =
    useGetUserTokensRegistryQuery(undefined, {
      selectFromResult: (result) => ({
        isFetchingTokens: result.isFetching,
        userTokensRegistry: result.data,
        visibleNfts: selectAllVisibleUserNFTsFromQueryResult(result),
        hiddenNfts: selectHiddenNftsFromQueryResult(result)
      })
    })
  const hiddenNftsIds =
    userTokensRegistry?.nonFungibleHiddenTokenIds ?? emptyTokenIdsList
  const userNonSpamNftIds =
    userTokensRegistry?.nonSpamTokenIds ?? emptyTokenIdsList

  const { data: spamTokenBalancesRegistry, isLoading: isLoadingSpamBalances } =
    useBalancesFetcher({
      accounts,
      networks,
      isSpamRegistry: true
    })

  const { data: tokenBalancesRegistry, isLoading: isLoadingTokenBalances } =
    // will fetch balances for all accounts so we can filter NFTs by accounts
    useBalancesFetcher(
      isFetchingTokens || networks.length === 0 || allAccounts.length === 0
        ? skipToken
        : {
            accounts: allAccounts,
            networks
          }
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
      simpleHashSpamNfts,
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

  const selectedNftList =
    selectedTab === 'collected' ? visibleUserNonSpamNfts : hiddenAndSpamNfts

  const sortedSelectedNftList = React.useMemo(() => {
    return selectedNftList.slice().sort(compareTokensByName)
  }, [selectedNftList])

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
      selectedTab === 'collected' ? null : spamTokenBalancesRegistry,
      hideUnownedNfts
    )
  }, [
    accounts,
    allAccounts,
    hideUnownedNfts,
    sortedSelectedNftListForChains,
    spamTokenBalancesRegistry,
    tokenBalancesRegistry,
    selectedTab
  ])

  // differed queries
  const {
    data: assetIdsByCollectionNameRegistryInfo,
    isFetching: isFetchingAssetIdsByCollectionNameRegistryInfo
  } = useGetNftAssetIdsByCollectionRegistryQuery(
    sortedSelectedNftListForChainsAndAccounts.length && groupNftsByCollection
      ? sortedSelectedNftListForChainsAndAccounts
      : skipToken
  )

  const assetIdsByCollectionNameRegistry =
    assetIdsByCollectionNameRegistryInfo?.registry
  const isFetchingLatestAssetIdsByCollectionNameRegistry =
    assetIdsByCollectionNameRegistryInfo?.isStreaming ??
    isFetchingAssetIdsByCollectionNameRegistryInfo ??
    false

  const collectionNames = React.useMemo(() => {
    return Object.keys(assetIdsByCollectionNameRegistry ?? {})
  }, [assetIdsByCollectionNameRegistry])

  const { nftCollectionAssets, assetsByCollectionName } = React.useMemo(() => {
    const nftCollectionAssets: BraveWallet.BlockchainToken[] = []
    const assetsByCollectionName: Record<
      string,
      BraveWallet.BlockchainToken[]
    > = {}

    // skip if not in "group by collection" mode
    if (!groupNftsByCollection) {
      return {
        nftCollectionAssets,
        assetsByCollectionName
      }
    }

    // skip if registry is not ready
    if (!assetIdsByCollectionNameRegistry) {
      return {
        nftCollectionAssets,
        assetsByCollectionName
      }
    }

    for (const token of sortedSelectedNftListForChainsAndAccounts) {
      const collectionNameForToken = getTokenCollectionName(
        collectionNames,
        assetIdsByCollectionNameRegistry,
        token
      )

      // create collection and collection token if it doesn't exist
      if (!assetsByCollectionName[collectionNameForToken]) {
        // add collection asset to the list
        const collectionToken = {
          ...token,
          tokenId: '',
          name: collectionNameForToken
        }
        nftCollectionAssets.push(collectionToken)

        // create collection in assets map
        assetsByCollectionName[collectionNameForToken] = []
      }

      // add token to the collection
      assetsByCollectionName[collectionNameForToken].push(token)
    }

    // sort collections by name
    nftCollectionAssets.sort(compareTokensByName)

    return { nftCollectionAssets, assetsByCollectionName }
  }, [
    groupNftsByCollection,
    assetIdsByCollectionNameRegistry,
    sortedSelectedNftListForChainsAndAccounts,
    collectionNames
  ])

  const { searchResults, totalNftsFound } = React.useMemo(() => {
    if (groupNftsByCollection) {
      const { foundCollections, totalNftsFound } =
        searchNftCollectionsAndGetTotalNftsFound(
          searchValue,
          nftCollectionAssets,
          assetsByCollectionName
        )
      return {
        searchResults: foundCollections,
        totalNftsFound
      }
    }

    const searchResults = searchNfts(
      searchValue,
      sortedSelectedNftListForChainsAndAccounts
    )
    return {
      searchResults,
      totalNftsFound: searchResults.length
    }
  }, [
    assetsByCollectionName,
    groupNftsByCollection,
    nftCollectionAssets,
    searchValue,
    sortedSelectedNftListForChainsAndAccounts
  ])

  const lastPageNumber = getLastPageNumber(searchResults, LIST_PAGE_ITEM_COUNT)

  /** label summary is shown only on the selected tab */
  const dropDownOptions: NftDropdownOption[] = React.useMemo(() => {
    return [
      {
        id: 'collected',
        label: getLocale('braveNftsTabCollected'),
        labelSummary: totalNftsFound
      },
      {
        id: 'hidden',
        label: getLocale('braveNftsTabHidden'),
        labelSummary: totalNftsFound
      }
    ]
  }, [totalNftsFound])

  const renderedListPage = React.useMemo(() => {
    return getListPageItems(
      searchResults,
      currentPageNumber,
      LIST_PAGE_ITEM_COUNT
    )
  }, [searchResults, currentPageNumber])

  const isLoadingAssets =
    isLoadingTokenBalances ||
    !assetAutoDiscoveryCompleted ||
    (groupNftsByCollection &&
      isFetchingLatestAssetIdsByCollectionNameRegistry) ||
    (selectedTab === 'hidden' && (isLoadingSpamNfts || isLoadingSpamBalances))

  // methods
  const onSearchValueChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
      if (currentPageNumber !== 1) {
        history.push(makePortfolioNftsRoute(selectedTab, 1))
      }
    },
    [currentPageNumber, history, selectedTab]
  )

  const onSelectAsset = React.useCallback(
    (asset: BraveWallet.BlockchainToken) => {
      history.push(makePortfolioAssetRoute(true, getAssetIdKey(asset)))
      // reset nft metadata
      dispatch(WalletPageActions.updateNFTMetadata(undefined))
    },
    [dispatch, history]
  )

  const onSelectCollection = React.useCallback(
    (asset: BraveWallet.BlockchainToken) => {
      const collectionNameForToken = getTokenCollectionName(
        collectionNames,
        assetIdsByCollectionNameRegistry,
        asset
      )
      if (collectionNameForToken) {
        history.push(makePortfolioNftCollectionRoute(collectionNameForToken))
      }
    },
    [assetIdsByCollectionNameRegistry, collectionNames, history]
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

  const onSelectOption = React.useCallback(
    (selectedOption: NftDropdownOption) => {
      history.push(makePortfolioNftsRoute(selectedOption.id, 1))
    },
    [history]
  )

  const onCloseSearchBar = React.useCallback(() => {
    setShowSearchBar(false)
    setSearchValue('')
  }, [])

  const navigateToPage = React.useCallback(
    (pageNumber: number) => {
      history.push(makePortfolioNftsRoute(selectedTab, pageNumber))
      listScrollContainerRef.current?.scrollIntoView(scrollOptions)
    },
    [history, selectedTab]
  )

  // effects
  React.useEffect(() => {
    getAPIProxy().braveWalletP3A.recordNFTGalleryView(visibleNfts.length)
  }, [visibleNfts.length])

  // render
  return (
    <ContentWrapper
      fullWidth={true}
      fullHeight={isPanel}
      justifyContent='flex-start'
      isPanel={isPanel}
    >
      <ControlBarWrapper
        justifyContent='space-between'
        alignItems='center'
        showSearchBar={showSearchBar}
        isNFTView={true}
      >
        {!showSearchBar && (
          <NftDropdown
            selectedOptionId={selectedTab}
            options={dropDownOptions}
            onSelect={onSelectOption}
          />
        )}
        <Row width={showSearchBar ? '100%' : 'unset'}>
          {showSearchBar ? (
            <Row width='unset'>
              <SearchBarWrapper
                margin='0px 12px 0px 0px'
                showSearchBar={showSearchBar}
              >
                <SearchBar
                  placeholder={getLocale('braveWalletSearchText')}
                  action={onSearchValueChange}
                  value={searchValue}
                  isV2={true}
                  autoFocus={true}
                />
              </SearchBarWrapper>
              <PortfolioActionButton onClick={onCloseSearchBar}>
                <ButtonIcon name='close' />
              </PortfolioActionButton>
            </Row>
          ) : (
            <Row
              width='unset'
              gap='12px'
            >
              <PortfolioActionButton onClick={() => setShowSearchBar(true)}>
                <ButtonIcon name='search' />
              </PortfolioActionButton>
              <PortfolioActionButton onClick={toggleShowAddNftModal}>
                <ButtonIcon name='plus-add' />
              </PortfolioActionButton>
              <PortfolioActionButton onClick={onShowPortfolioSettings}>
                <ButtonIcon name='filter-settings' />
              </PortfolioActionButton>
            </Row>
          )}
        </Row>
      </ControlBarWrapper>
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
              {groupNftsByCollection
                ? renderedListPage.map((collectionToken) => {
                    return (
                      <NftCollectionGridViewItem
                        key={collectionToken.name}
                        collectionToken={collectionToken}
                        tokensInCollection={
                          assetsByCollectionName[collectionToken.name]
                        }
                        onSelectAsset={onSelectCollection}
                      />
                    )
                  })
                : renderedListPage.map((nft) => {
                    const assetId = getAssetIdKey(nft)
                    const isSpam =
                      nft.isSpam || allSpamNftsIds.includes(assetId)
                    const isHidden =
                      !nft.visible ||
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
  )
}
