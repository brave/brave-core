// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router-dom'
import { useDispatch } from 'react-redux'

// types
import { BraveWallet, WalletRoutes } from '../../../../../constants/types'

// hooks
import { useNftPin } from '../../../../../common/hooks/nft-pin'

// selectors
import {
  useSafeUISelector,
  useSafeWalletSelector,
  useUnsafeWalletSelector
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
  useSetNftDiscoveryEnabledMutation
} from '../../../../../common/slices/api.slice'
import { getBalance } from '../../../../../utils/balance-utils'
import {
  AccountsGroupByOption,
  NetworksGroupByOption
} from '../../../../../options/group-assets-by-options'
import { useApiProxy } from '../../../../../common/hooks/use-api-proxy'
import { getAssetIdKey } from '../../../../../utils/asset-utils'
import { useQuery } from '../../../../../common/hooks/use-query'

// components
import SearchBar from '../../../../shared/search-bar'
import { NFTGridViewItem } from '../../portfolio/components/nft-grid-view/nft-grid-view-item'
import { EnableNftDiscoveryModal } from '../../../popup-modals/enable-nft-discovery-modal/enable-nft-discovery-modal'
import { AutoDiscoveryEmptyState } from './auto-discovery-empty-state/auto-discovery-empty-state'
import { NftIpfsBanner } from '../../../nft-ipfs-banner/nft-ipfs-banner'

// styles
import { NftGrid } from './nfts.styles'
import { Row, ScrollableColumn, VerticalSpace } from '../../../../shared/style'
import { AddOrEditNftModal } from '../../../popup-modals/add-edit-nft-modal/add-edit-nft-modal'
import { NftsEmptyState } from './nfts-empty-state/nfts-empty-state'
import {
  ButtonIcon,
  CircleButton,
  SearchBarWrapper,
  ControlBarWrapper,
  ContentWrapper
} from '../../portfolio/style'
import { AssetGroupContainer } from '../../../asset-group-container/asset-group-container'
import { networkEntityAdapter } from '../../../../../common/slices/entities/network.entity'
import {
  TokenBalancesRegistry //
} from '../../../../../common/slices/entities/token-balance.entity'
import { NftGridViewItemSkeleton } from '../../portfolio/components/nft-grid-view/nft-grid-view-item-skeleton'
import {
  NftDropdown,
  NftDropdownOption,
  NftDropdownOptionId
} from './nft-group-selector/nft-group-selector'

interface Props {
  networks: BraveWallet.NetworkInfo[]
  nftList: BraveWallet.BlockchainToken[]
  accounts: BraveWallet.AccountInfo[]
  onShowPortfolioSettings?: () => void
  tokenBalancesRegistry: TokenBalancesRegistry | undefined
}

const compareFn = (
  a: BraveWallet.BlockchainToken,
  b: BraveWallet.BlockchainToken
) => a.name.localeCompare(b.name)

export const Nfts = (props: Props) => {
  const {
    nftList,
    accounts,
    networks,
    onShowPortfolioSettings,
    tokenBalancesRegistry
  } = props

  const { braveWalletP3A } = useApiProxy()

  // redux
  const isNftPinningFeatureEnabled = useSafeWalletSelector(
    WalletSelectors.isNftPinningFeatureEnabled
  )
  const hiddenNfts = useUnsafeWalletSelector(
    WalletSelectors.removedNonFungibleTokens
  )
  const selectedGroupAssetsByItem = useSafeWalletSelector(
    WalletSelectors.selectedGroupAssetsByItem
  )
  const assetAutoDiscoveryCompleted = useSafeWalletSelector(
    WalletSelectors.assetAutoDiscoveryCompleted
  )
  const isRefreshingTokens = useSafeWalletSelector(
    WalletSelectors.isRefreshingNetworksAndTokens
  )
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const deletedNonFungibleTokenIds = useUnsafeWalletSelector(
    WalletSelectors.deletedNonFungibleTokenIds
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

  // hooks
  const history = useHistory()
  const dispatch = useDispatch()
  const { nonFungibleTokens, isIpfsBannerVisible, onToggleShowIpfsBanner } =
    useNftPin()
  const urlSearchParams = useQuery()
  const tab = urlSearchParams.get('tab')
  const selectedTab: NftDropdownOptionId =
    tab === 'collected' || tab === 'hidden' ? tab : 'collected'

  // queries
  const { data: isNftAutoDiscoveryEnabled } =
    useGetNftDiscoveryEnabledStatusQuery()
  const { data: simpleHashSpamNfts = [] } = useGetSimpleHashSpamNftsQuery()

  // mutations
  const [setNftDiscovery] = useSetNftDiscoveryEnabledMutation()

  // methods
  const onSearchValueChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
    },
    []
  )

  const onSelectAsset = React.useCallback(
    (asset: BraveWallet.BlockchainToken) => {
      history.push(
        `${
          WalletRoutes.PortfolioNFTs //
        }/${
          asset.chainId //
        }/${
          asset.contractAddress //
        }/${asset.tokenId}`
      )
      // reset nft metadata
      dispatch(WalletPageActions.updateNFTMetadata(undefined))
    },
    [dispatch]
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
  }, [])

  const onSelectOption = React.useCallback(
    (selectedOption: NftDropdownOption) => {
      history.push({
        pathname: WalletRoutes.PortfolioNFTs,
        search: `?tab=${selectedOption.id}`
      })
    },
    []
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

  React.useEffect(() => {
    braveWalletP3A.recordNFTGalleryView(nftList.length)
  }, [nftList])

  // memos
  const [userNonSpamNfts, userMarkedSpamNfts] = React.useMemo(() => {
    return nftList.reduce(
      (
        result: [BraveWallet.BlockchainToken[], BraveWallet.BlockchainToken[]],
        nft: BraveWallet.BlockchainToken
      ) => {
        if (!nft.isSpam) {
          result[0].push(nft)
        } else {
          result[1].push(nft)
        }
        return result
      },
      [[], []]
    )
  }, [nftList])

  const [hiddenNftsIds, userNonSpamNftIds] = React.useMemo(() => {
    return [
      hiddenNfts.map((nft) => getAssetIdKey(nft)),
      userNonSpamNfts.map((nft) => getAssetIdKey(nft))
    ]
  }, [hiddenNfts, userNonSpamNfts])

  const [allSpamNfts, allSpamNftsIds] = React.useMemo(() => {
    // filter out NFTs user has marked not spam
    // hidden NFTs,
    // and deleted NFTs
    const excludedNftIds = userNonSpamNftIds
      .concat(hiddenNftsIds)
      .concat(deletedNonFungibleTokenIds)
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
    deletedNonFungibleTokenIds
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

  // Returns a list of assets based on provided account
  const getFilteredNftsByAccount = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      return renderedList.filter(
        (nft) =>
          nft.coin === account.accountId.coin &&
          new Amount(
            getBalance(account.accountId, nft, tokenBalancesRegistry)
          ).gte('1')
      )
    },
    [renderedList, tokenBalancesRegistry]
  )

  // Returns a list of assets based on provided network
  const getAssetsByNetwork = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      return renderedList.filter(
        (asset) =>
          networkEntityAdapter
            .selectId({
              chainId: asset.chainId,
              coin: asset.coin
            })
            .toString() === networkEntityAdapter.selectId(network).toString()
      )
    },
    [renderedList]
  )

  const renderGridViewItem = React.useCallback(
    (nft: BraveWallet.BlockchainToken) => {
      const assetId = getAssetIdKey(nft)

      return (
        <NFTGridViewItem
          key={assetId}
          token={nft}
          onSelectAsset={() => onSelectAsset(nft)}
          isTokenHidden={
            hiddenNftsIds.includes(assetId) || allSpamNftsIds.includes(assetId)
          }
          isTokenSpam={allSpamNftsIds.includes(assetId)}
        />
      )
    },
    [hiddenNftsIds, allSpamNftsIds, onSelectAsset]
  )

  const listUiByAccounts = React.useMemo(() => {
    return accounts.map((account) => (
      <Row
        width='100%'
        key={account.accountId.uniqueKey}
      >
        {getFilteredNftsByAccount(account).length !== 0 && (
          <AssetGroupContainer
            key={account.accountId.uniqueKey}
            balance=''
            hideBalance={true}
            account={account}
            isDisabled={getFilteredNftsByAccount(account).length === 0}
            hasBorder={false}
          >
            <VerticalSpace space='16px' />
            <NftGrid>
              {getFilteredNftsByAccount(account).map(renderGridViewItem)}
              {!assetAutoDiscoveryCompleted && <NftGridViewItemSkeleton />}
            </NftGrid>
          </AssetGroupContainer>
        )}
      </Row>
    ))
  }, [accounts, getFilteredNftsByAccount, onSelectAsset])

  const listUiByNetworks = React.useMemo(() => {
    return networks?.map((network) => (
      <Row
        width='100%'
        key={networkEntityAdapter.selectId(network).toString()}
      >
        {getAssetsByNetwork(network).length !== 0 && (
          <AssetGroupContainer
            balance=''
            hideBalance={true}
            network={network}
            isDisabled={getAssetsByNetwork(network).length === 0}
            hasBorder={false}
          >
            <VerticalSpace space='16px' />
            <NftGrid>
              {getAssetsByNetwork(network).map(renderGridViewItem)}
              {!assetAutoDiscoveryCompleted && <NftGridViewItemSkeleton />}
            </NftGrid>
          </AssetGroupContainer>
        )}
      </Row>
    ))
  }, [getAssetsByNetwork, networks])

  const listUi = React.useMemo(() => {
    return selectedGroupAssetsByItem === NetworksGroupByOption.id ? (
      listUiByNetworks
    ) : selectedGroupAssetsByItem === AccountsGroupByOption.id ? (
      listUiByAccounts
    ) : (
      <>
        <VerticalSpace space='16px' />
        <NftGrid>
          {renderedList.map(renderGridViewItem)}
          {!assetAutoDiscoveryCompleted && <NftGridViewItemSkeleton />}
        </NftGrid>
      </>
    )
  }, [
    listUiByAccounts,
    listUiByNetworks,
    selectedGroupAssetsByItem,
    renderedList
  ])

  // effects
  React.useEffect(() => {
    dispatch(WalletActions.refreshNetworksAndTokens({}))
  }, [assetAutoDiscoveryCompleted])

  return (
    <ContentWrapper
      fullWidth={true}
      fullHeight={isPanel}
      justifyContent='flex-start'
      isPanel={isPanel}
    >
      {isNftPinningFeatureEnabled &&
      isIpfsBannerVisible &&
      nonFungibleTokens.length > 0 ? (
        <Row
          justifyContent='center'
          alignItems='center'
          padding='0px 32px'
          marginBottom={16}
        >
          <NftIpfsBanner onDismiss={onToggleShowIpfsBanner} />
        </Row>
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
              <CircleButton onClick={onCloseSearchBar}>
                <ButtonIcon name='close' />
              </CircleButton>
            </Row>
          ) : (
            <Row width='unset'>
              <CircleButton
                marginRight={12}
                onClick={() => setShowSearchBar(true)}
              >
                <ButtonIcon name='search' />
              </CircleButton>
              {isNftPinningFeatureEnabled && nonFungibleTokens.length > 0 ? (
                <CircleButton
                  onClick={onClickIpfsButton}
                  marginRight={12}
                >
                  <ButtonIcon name='product-ipfs-outline' />
                </CircleButton>
              ) : null}
              <CircleButton
                onClick={toggleShowAddNftModal}
                marginRight={12}
              >
                <ButtonIcon name='plus-add' />
              </CircleButton>
              <CircleButton onClick={onShowPortfolioSettings}>
                <ButtonIcon name='filter-settings' />
              </CircleButton>
            </Row>
          )}
        </Row>
      </ControlBarWrapper>
      <ScrollableColumn padding='0px 20px 20px 20px'>
        {nftList.length === 0 && hiddenNfts.length === 0 ? (
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
          listUi
        )}
      </ScrollableColumn>
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
