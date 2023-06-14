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
  WalletAccountType,
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
import { getBalance } from '../../../../../utils/balance-utils'

// components
import SearchBar from '../../../../shared/search-bar'
import { NFTGridViewItem } from '../../portfolio/components/nft-grid-view/nft-grid-view-item'
import { EnableNftDiscoveryModal } from '../../../popup-modals/enable-nft-discovery-modal/enable-nft-discovery-modal'
import { AutoDiscoveryEmptyState } from './auto-discovery-empty-state/auto-discovery-empty-state'
import { TabOption, Tabs } from '../../../../shared/tabs/tabs'

// styles
import {
  NftGrid
} from './nfts.styles'
import { Row, ScrollableColumn } from '../../../../shared/style'
import { AddOrEditNftModal } from '../../../popup-modals/add-edit-nft-modal/add-edit-nft-modal'
import { NftsEmptyState } from './nfts-empty-state/nfts-empty-state'
import { ButtonIcon, CircleButton } from '../../portfolio/style'
import { AssetGroupContainer } from '../../../asset-group-container/asset-group-container'

interface Props {
  networks: BraveWallet.NetworkInfo[]
  nftList: BraveWallet.BlockchainToken[],
  accounts: WalletAccountType[]
  onToggleShowIpfsBanner: () => void
  onShowPortfolioSettings?: () => void
}

export const Nfts = (props: Props) => {
  const {
    nftList,
    accounts,
    onToggleShowIpfsBanner,
    onShowPortfolioSettings
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

  // Returns a list of assets based on provided account
  const getFilteredNftsByAccount = React.useCallback(
    (account: WalletAccountType) => {
      return renderedList.filter(
        (nft) =>
          nft.coin === account.accountId.coin &&
          new Amount(getBalance(account, nft)).gte('1')
      )
    }, [renderedList])

  const listUiByAccounts = React.useMemo(() => {
    return accounts.map((account) => (
      <>
        {getFilteredNftsByAccount(account).length !== 0 && (
          <AssetGroupContainer
            key={account.address}
            balance=''
            hideBalance={true}
            account={account}
            isDisabled={getFilteredNftsByAccount(account).length === 0}
            hasBorder={false}
          >
            <NftGrid>
              {getFilteredNftsByAccount(account).map((nft) => (
                <NFTGridViewItem
                  isHidden={selectedTab === 'hidden'}
                  key={`${nft.tokenId}-${nft.contractAddress}`}
                  token={nft}
                  onSelectAsset={() => onSelectAsset(nft)}
                />
              ))}
            </NftGrid>
          </AssetGroupContainer>
        )}
      </>
    ))
  }, [renderedList, accounts, getFilteredNftsByAccount, onSelectAsset])

  return (
    <>
      <Row
        justifyContent='space-between'
        alignItems='center'
        padding='0px 32px'
        marginBottom={12}
      >
        <Tabs options={tabOptions} onSelect={onSelectTab} />
        <Row  width='unset'>
          <Row style={{ width: 230 }} margin='0px 12px 0px 0px'>
            <SearchBar
              placeholder={getLocale('braveWalletSearchText')}
              action={onSearchValueChange}
              value={searchValue}
              isV2={true}
            />
          </Row>
          {isNftPinningFeatureEnabled && nonFungibleTokens.length > 0 && (
            <CircleButton
              onClick={onClickIpfsButton}
              marginRight={12}
            >
              <ButtonIcon name='product-ipfs-outline' />
            </CircleButton>
          )}
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
      </Row>

      <ScrollableColumn padding='10px 20px 20px 20px'>
        {nftList.length === 0 && hiddenNfts.length === 0 ? (
          isNftAutoDiscoveryEnabled ? (
            <AutoDiscoveryEmptyState
              onImportNft={toggleShowAddNftModal}
              onRefresh={onRefresh}
            />
          ) : (
            <NftsEmptyState onImportNft={toggleShowAddNftModal} />
          )
        ) : listUiByAccounts}
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
