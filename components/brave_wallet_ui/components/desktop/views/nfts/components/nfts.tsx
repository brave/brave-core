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
import { useNftPin } from '../../../../../common/hooks/nft-pin'
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
import { getLocale } from '$web-common/locale'
import Amount from '../../../../../utils/amount'
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../../common/constants/local-storage-keys'
import {
  useGetNftDiscoveryEnabledStatusQuery,
  useGetSimpleHashSpamNftsQuery,
  useGetUserTokensRegistryQuery,
  useSetNftDiscoveryEnabledMutation
} from '../../../../../common/slices/api.slice'
import { useApiProxy } from '../../../../../common/hooks/use-api-proxy'
import {
  getAssetIdKey,
  getTokensWithBalanceForAccounts
} from '../../../../../utils/asset-utils'
import { useQuery } from '../../../../../common/hooks/use-query'
import {
  makePortfolioAssetRoute,
  makePortfolioNftsRoute
} from '../../../../../utils/routes-utils'
import {
  selectAllVisibleUserNFTsFromQueryResult,
  selectHiddenNftsFromQueryResult //
} from '../../../../../common/slices/entities/blockchain-token.entity'

// components
import SearchBar from '../../../../shared/search-bar'
import { NFTGridViewItem } from '../../portfolio/components/nft-grid-view/nft-grid-view-item'
import { EnableNftDiscoveryModal } from '../../../popup-modals/enable-nft-discovery-modal/enable-nft-discovery-modal'
import { AutoDiscoveryEmptyState } from './auto-discovery-empty-state/auto-discovery-empty-state'
import { NftIpfsBanner } from '../../../nft-ipfs-banner/nft-ipfs-banner'
import {
  NftGridViewItemSkeleton //
} from '../../portfolio/components/nft-grid-view/nft-grid-view-item-skeleton'
import { Pagination } from '../../../../shared/pagination/pagination'
import {
  NftDropdown,
  NftDropdownOption
} from './nft-group-selector/nft-group-selector'

// styles
import { BannerWrapper, NFTListWrapper, NftGrid } from './nfts.styles'
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

const compareFn = (
  a: BraveWallet.BlockchainToken,
  b: BraveWallet.BlockchainToken
) => a.name.localeCompare(b.name)

const searchNfts = (
  searchValue: string,
  items: BraveWallet.BlockchainToken[]
) => {
  if (searchValue === '') {
    return items
  }

  return items.filter((item) => {
    const tokenId = new Amount(item.tokenId).toNumber().toString()
    const searchValueLower = searchValue.toLowerCase()
    return (
      item.name.toLocaleLowerCase().includes(searchValueLower) ||
      item.symbol.toLocaleLowerCase().includes(searchValueLower) ||
      tokenId.includes(searchValueLower)
    )
  })
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
  const isNftPinningFeatureEnabled = useSafeWalletSelector(
    WalletSelectors.isNftPinningFeatureEnabled
  )
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
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showAddNftModal, setShowAddNftModal] = React.useState<boolean>(false)
  const [showNftDiscoveryModal, setShowNftDiscoveryModal] =
    React.useState<boolean>(
      localStorage.getItem(
        LOCAL_STORAGE_KEYS.IS_ENABLE_NFT_AUTO_DISCOVERY_MODAL_HIDDEN
      ) === null
    )
  const [showSearchBar, setShowSearchBar] = React.useState<boolean>(false)

  // custom hooks
  const { braveWalletP3A } = useApiProxy()
  const { isIpfsBannerVisible, onToggleShowIpfsBanner } = useNftPin()

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

  const shouldFetchSpamNftBalances =
    selectedTab === 'hidden' &&
    !isLoadingSpamNfts &&
    !hideUnownedNfts &&
    accounts.length > 0 &&
    networks.length > 0

  const { data: spamTokenBalancesRegistry } = useBalancesFetcher(
    shouldFetchSpamNftBalances
      ? {
          accounts,
          networks,
          isSpamRegistry: true
        }
      : skipToken
  )

  const { data: tokenBalancesRegistry } =
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
      const results: {
        visibleUserNonSpamNfts: BraveWallet.BlockchainToken[]
        visibleUserMarkedSpamNfts: BraveWallet.BlockchainToken[]
      } = {
        visibleUserNonSpamNfts: [],
        visibleUserMarkedSpamNfts: []
      }
      for (const nft of visibleNfts) {
        if (nft.isSpam) {
          results.visibleUserMarkedSpamNfts.push(nft)
        } else {
          if (nft.visible) {
            results.visibleUserNonSpamNfts.push(nft)
          }
        }
      }
      return results
    }, [visibleNfts])

  const hiddenNftsIds =
    userTokensRegistry?.nonFungibleHiddenTokenIds ?? emptyTokenIdsList
  const userNonSpamNftIds =
    userTokensRegistry?.nonSpamTokenIds ?? emptyTokenIdsList

  const [allSpamNfts, allSpamNftsIds] = React.useMemo(() => {
    // filter out NFTs user has marked not spam
    // hidden NFTs, and deleted NFTs
    const excludedNftIds = userNonSpamNftIds
      .concat(hiddenNftsIds)
      .concat(userTokensRegistry?.deletedTokenIds || [])
    const simpleHashList = simpleHashSpamNfts.filter(
      (nft) => !excludedNftIds.includes(getAssetIdKey(nft))
    )
    const simpleHashListIds = simpleHashList.map((nft) => getAssetIdKey(nft))
    // add NFTs user has marked as NFT if they are not in the list
    // to avoid duplicates
    const fullSpamList = [
      ...simpleHashList,
      ...visibleUserMarkedSpamNfts.filter(
        (nft) => !simpleHashListIds.includes(getAssetIdKey(nft))
      )
    ]

    return [fullSpamList, fullSpamList.map((nft) => getAssetIdKey(nft))]
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
    return selectedNftList.slice().sort(compareFn)
  }, [selectedNftList])

  // Filters the user's tokens based on the users
  // filteredOutPortfolioNetworkKeys pref and visible networks.
  const sortedSelectedNftListForChains = React.useMemo(() => {
    return sortedSelectedNftList.filter((token) =>
      networks.some(
        (net) => net.chainId === token.chainId && net.coin === token.coin
      )
    )
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

  const { searchResults, totalNftsFound } = React.useMemo(() => {
    const searchResults = searchNfts(
      searchValue,
      sortedSelectedNftListForChainsAndAccounts
    )
    return {
      searchResults,
      totalNftsFound: searchResults.length
    }
  }, [searchValue, sortedSelectedNftListForChainsAndAccounts])

  const lastPageNumber =
    Math.floor(searchResults.length / LIST_PAGE_ITEM_COUNT) + 1

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
    const pageStartItemIndex =
      currentPageNumber * LIST_PAGE_ITEM_COUNT - LIST_PAGE_ITEM_COUNT
    return searchResults.slice(
      pageStartItemIndex,
      pageStartItemIndex + LIST_PAGE_ITEM_COUNT
    )
  }, [searchResults, currentPageNumber])

  const isLoadingAssets =
    !assetAutoDiscoveryCompleted ||
    (selectedTab === 'hidden' &&
      (isLoadingSpamNfts ||
        (shouldFetchSpamNftBalances && !spamTokenBalancesRegistry)))

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

  const onClickIpfsButton = React.useCallback(() => {
    onToggleShowIpfsBanner()
  }, [onToggleShowIpfsBanner])

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
    dispatch(WalletActions.refreshNetworksAndTokens({}))
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
    braveWalletP3A.recordNFTGalleryView(visibleNfts.length)
  }, [braveWalletP3A, visibleNfts.length])

  return (
    <ContentWrapper
      fullWidth={true}
      fullHeight={isPanel}
      justifyContent='flex-start'
      isPanel={isPanel}
    >
      {isNftPinningFeatureEnabled &&
      isIpfsBannerVisible &&
      visibleNfts.length > 0 ? (
        <BannerWrapper
          justifyContent='center'
          alignItems='center'
          marginBottom={16}
        >
          <NftIpfsBanner onDismiss={onToggleShowIpfsBanner} />
        </BannerWrapper>
      ) : null}

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
              {isNftPinningFeatureEnabled && visibleNfts.length > 0 ? (
                <PortfolioActionButton onClick={onClickIpfsButton}>
                  <ButtonIcon name='product-ipfs-outline' />
                </PortfolioActionButton>
              ) : null}
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
              {renderedListPage.map((nft) => {
                const assetId = getAssetIdKey(nft)
                const isSpam = nft.isSpam || allSpamNftsIds.includes(assetId)
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
