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
  NftDropdownOptionId,
  WalletRoutes
} from '../../../../../constants/types'

// hooks
import { useNftPin } from '../../../../../common/hooks/nft-pin'

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
import { getAssetIdKey } from '../../../../../utils/asset-utils'
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
import { Pagination } from '../../../../shared/pagination/pagination'

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
import { NftGridViewItemSkeleton } from '../../portfolio/components/nft-grid-view/nft-grid-view-item-skeleton'
import {
  NftDropdown,
  NftDropdownOption
} from './nft-group-selector/nft-group-selector'

interface Props {
  nftList: BraveWallet.BlockchainToken[]
  onShowPortfolioSettings?: () => void
}

const compareFn = (
  a: BraveWallet.BlockchainToken,
  b: BraveWallet.BlockchainToken
) => a.name.localeCompare(b.name)

const LIST_PAGE_ITEM_COUNT = 15

const scrollOptions: ScrollIntoViewOptions = { block: 'start' }

export const Nfts = ({ nftList, onShowPortfolioSettings }: Props) => {
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
  const { data: simpleHashSpamNfts = [] } = useGetSimpleHashSpamNftsQuery()
  const { userTokensRegistry, hiddenNfts, visibleNfts } =
    useGetUserTokensRegistryQuery(undefined, {
      selectFromResult: (result) => ({
        userTokensRegistry: result.data,
        visibleNfts: selectAllVisibleUserNFTsFromQueryResult(result),
        hiddenNfts: selectHiddenNftsFromQueryResult(result)
      })
    })

  // mutations
  const [setNftDiscovery] = useSetNftDiscoveryEnabledMutation()

  // methods
  const onSearchValueChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
      if (currentPageNumber !== 1) {
        history.push(makePortfolioNftsRoute(selectedTab, 1))
      }
    },
    [history, currentPageNumber, selectedTab]
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
      history.push({
        pathname: WalletRoutes.PortfolioNFTs,
        search: `?tab=${selectedOption.id}`
      })
    },
    [history]
  )

  const searchNfts = React.useCallback(
    (item: BraveWallet.BlockchainToken) => {
      const tokenId = new Amount(item.tokenId).toNumber().toString()

      return (
        item.name.toLowerCase() === searchValue.toLowerCase() ||
        item.name.toLowerCase().includes(searchValue.toLowerCase()) ||
        item.symbol.toLocaleLowerCase() === searchValue.toLowerCase() ||
        item.symbol.toLowerCase().includes(searchValue.toLowerCase()) ||
        tokenId === searchValue.toLowerCase() ||
        tokenId.includes(searchValue.toLowerCase())
      )
    },
    [searchValue]
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

  // memos
  const { userNonSpamNfts, userMarkedSpamNfts } = React.useMemo(() => {
    const results: {
      userNonSpamNfts: BraveWallet.BlockchainToken[]
      userMarkedSpamNfts: BraveWallet.BlockchainToken[]
    } = {
      userNonSpamNfts: [],
      userMarkedSpamNfts: []
    }
    for (const nft of nftList) {
      if (nft.isSpam) {
        results.userMarkedSpamNfts.push(nft)
      } else {
        if (nft.visible) {
          results.userNonSpamNfts.push(nft)
        }
      }
    }
    return results
  }, [nftList])

  const [hiddenNftsIds, userNonSpamNftIds] = React.useMemo(() => {
    if (!userTokensRegistry) {
      return [[], []]
    }
    return [
      userTokensRegistry.nonFungibleHiddenTokenIds,
      userTokensRegistry.nonSpamTokenIds
    ]
  }, [userTokensRegistry])

  const [allSpamNfts, allSpamNftsIds] = React.useMemo(() => {
    // filter out NFTs user has marked not spam
    // hidden NFTs,
    // and deleted NFTs
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
      ...userMarkedSpamNfts.filter(
        (nft) => !simpleHashListIds.includes(getAssetIdKey(nft))
      )
    ]

    return [fullSpamList, fullSpamList.map((nft) => getAssetIdKey(nft))]
  }, [
    userMarkedSpamNfts,
    simpleHashSpamNfts,
    hiddenNftsIds,
    userNonSpamNftIds,
    userTokensRegistry
  ])

  const [sortedNfts, sortedHiddenNfts, sortedSpamNfts] = React.useMemo(() => {
    if (searchValue === '') {
      return [
        userNonSpamNfts.slice().sort(compareFn),
        hiddenNfts.slice().sort(compareFn),
        allSpamNfts.slice().sort(compareFn)
      ]
    }

    return [
      userNonSpamNfts.filter(searchNfts).sort(compareFn),
      hiddenNfts.filter(searchNfts).sort(compareFn),
      allSpamNfts.filter(searchNfts).sort(compareFn)
    ]
  }, [searchValue, userNonSpamNfts, hiddenNfts, allSpamNfts, searchNfts])

  const dropDownOptions: NftDropdownOption[] = React.useMemo(() => {
    return [
      {
        id: 'collected',
        label: getLocale('braveNftsTabCollected'),
        labelSummary: sortedNfts.length
      },
      {
        id: 'hidden',
        label: getLocale('braveNftsTabHidden'),
        labelSummary: sortedHiddenNfts.concat(sortedSpamNfts).length
      }
    ]
  }, [sortedHiddenNfts, sortedSpamNfts, sortedNfts])

  const renderedList = React.useMemo(() => {
    switch (selectedTab) {
      case 'collected':
        return sortedNfts
      case 'hidden':
        return sortedHiddenNfts.concat(sortedSpamNfts)
      default:
        return sortedNfts
    }
  }, [selectedTab, sortedNfts, sortedHiddenNfts, sortedSpamNfts])

  const renderedListPage = React.useMemo(() => {
    const pageStartItemIndex =
      currentPageNumber * LIST_PAGE_ITEM_COUNT - LIST_PAGE_ITEM_COUNT
    return renderedList.slice(
      pageStartItemIndex,
      pageStartItemIndex + LIST_PAGE_ITEM_COUNT
    )
  }, [renderedList, currentPageNumber])

  // computed
  const lastPageNumber =
    Math.floor(renderedList.length / LIST_PAGE_ITEM_COUNT) + 1

  // effects
  React.useEffect(() => {
    braveWalletP3A.recordNFTGalleryView(nftList.length)
  }, [braveWalletP3A, nftList])

  React.useEffect(() => {
    dispatch(WalletActions.refreshNetworksAndTokens({}))
  }, [assetAutoDiscoveryCompleted, dispatch])

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
        {nftList.length === 0 &&
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

                return (
                  <NFTGridViewItem
                    key={assetId}
                    token={nft}
                    onSelectAsset={onSelectAsset}
                    isTokenHidden={
                      isSpam ||
                      Boolean(
                        userTokensRegistry?.nonFungibleHiddenTokenIds.includes(
                          assetId
                        )
                      )
                    }
                    isTokenSpam={isSpam}
                  />
                )
              })}
              {!assetAutoDiscoveryCompleted && <NftGridViewItemSkeleton />}
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
