// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation } from 'react-router'

// Types
import { WalletRoutes } from '../../../../constants/types'

// Options
import {
  AssetFilterOptions,
  HighToLowAssetsFilterOption
} from '../../../../options/asset-filter-options'
import {
  GroupAssetsByOptions,
  NoneGroupByOption
} from '../../../../options/group-assets-by-options'

// Constants
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../common/constants/local-storage-keys'
import {
  HIDE_SMALL_BALANCES_FIAT_THRESHOLD //
} from '../../../../common/constants/magics'

// Utils
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'
import {
  useGetDefaultFiatCurrencyQuery //
} from '../../../../common/slices/api.slice'
import {
  useLocalStorage,
  useSyncedLocalStorage
} from '../../../../common/hooks/use_local_storage'
import {
  makeInitialFilteredOutNetworkKeys //
} from '../../../../utils/local-storage-utils'

// Components
import { PopupModal } from '../../popup-modals/index'
import {
  FilterNetworksSection //
} from './filter-components/filter-networks-section'
import {
  FilterAccountsSection //
} from './filter-components/filter-accounts-section'
import { FilterToggleSection } from './filter-components/filter-toggle-section'
import {
  FilterDropdownSection //
} from './filter-components/filter-dropdown-section'

// Styles
import {
  VerticalDivider,
  VerticalSpacer,
  ScrollableColumn,
  HorizontalSpace,
  LeoSquaredButton
} from '../../../shared/style'
import { ContentWrapper, ButtonRow } from './portfolio-filters-modal.style'

interface Props {
  onClose: () => void
  onSave?: () => void
}

export const PortfolioFiltersModal = ({ onClose, onSave }: Props) => {
  // routing
  const { pathname: currentRoute } = useLocation()

  // Local-Storage
  // (not synced with other tabs to temporarily allow different portfolio views)
  const [filteredOutPortfolioNetworkKeys, setFilteredOutPortfolioNetworkKeys] =
    useLocalStorage(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_PORTFOLIO_NETWORK_KEYS,
      makeInitialFilteredOutNetworkKeys
    )
  const [filteredOutPortfolioAccountIds, setFilteredOutPortfolioAccountIds] =
    useLocalStorage<string[]>(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_PORTFOLIO_ACCOUNT_IDS,
      []
    )
  const [selectedGroupAssetsByItem, setSelectedGroupAssetsByItem] =
    useLocalStorage<string>(
      LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_ASSETS_BY,
      NoneGroupByOption.id
    )
  const [selectedAssetFilter, setSelectedAssetFilter] = useLocalStorage<string>(
    LOCAL_STORAGE_KEYS.PORTFOLIO_ASSET_FILTER_OPTION,
    HighToLowAssetsFilterOption.id
  )
  const [hidePortfolioSmallBalances, setHidePortfolioSmallBalances] =
    useLocalStorage<boolean>(
      LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_SMALL_BALANCES,
      false
    )
  const [groupNftsByCollection, setGroupNftsByCollection] =
    useLocalStorage<boolean>(
      LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_NFTS_BY_COLLECTION,
      false
    )

  // Synced Local-Storage
  const [showNetworkLogoOnNfts, setShowNetworkLogoOnNfts] =
    useSyncedLocalStorage<boolean>(
      LOCAL_STORAGE_KEYS.SHOW_NETWORK_LOGO_ON_NFTS,
      false
    )
  const [hideUnownedNfts, setHideUnownedNfts] = useSyncedLocalStorage<boolean>(
    LOCAL_STORAGE_KEYS.HIDE_UNOWNED_NFTS,
    false
  )

  // queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

  // State
  const [filteredOutNetworkKeys, setFilteredOutNetworkKeys] = React.useState<
    string[]
  >(filteredOutPortfolioNetworkKeys)
  const [filteredOutAccountIds, setFilteredOutAccountIds] = React.useState<
    string[]
  >(filteredOutPortfolioAccountIds)
  const [hideSmallBalances, setHideSmallBalances] = React.useState<boolean>(
    hidePortfolioSmallBalances
  )
  const [selectedAssetFilterOption, setSelectedAssetFilterOption] =
    React.useState<string>(selectedAssetFilter)
  const [selectedGroupAssetsByOption, setSelectedGroupAssetsByOption] =
    React.useState<string>(selectedGroupAssetsByItem)
  const [showNetworkLogo, setShowNetworkLogo] = React.useState(
    showNetworkLogoOnNfts
  )
  const [hideUnownedNftsToggle, setHideUnownedNftsToggle] =
    React.useState(hideUnownedNfts)
  const [groupNftsByCollectionToggle, setGroupNftsByCollectionToggle] =
    React.useState(groupNftsByCollection)

  // Memos
  const hideSmallBalancesDescription = React.useMemo(() => {
    const minAmount = new Amount(HIDE_SMALL_BALANCES_FIAT_THRESHOLD)
      .formatAsFiat(defaultFiatCurrency)
      .split('.')[0]
    return getLocale('braveWalletHideSmallBalancesDescription').replace(
      '$1',
      minAmount
    )
  }, [defaultFiatCurrency])

  // Computed
  const showNftFilters = currentRoute === WalletRoutes.PortfolioNFTs

  // Methods
  const onSaveChanges = React.useCallback(() => {
    setFilteredOutPortfolioNetworkKeys(filteredOutNetworkKeys)
    setFilteredOutPortfolioAccountIds(filteredOutAccountIds)
    setSelectedGroupAssetsByItem(selectedGroupAssetsByOption)
    setSelectedAssetFilter(selectedAssetFilterOption)
    setHidePortfolioSmallBalances(hideSmallBalances)
    setShowNetworkLogoOnNfts(showNetworkLogo)
    setHideUnownedNfts(hideUnownedNftsToggle)
    setGroupNftsByCollection(groupNftsByCollectionToggle)
    onSave?.()
    onClose()
  }, [
    setFilteredOutPortfolioNetworkKeys,
    filteredOutNetworkKeys,
    setFilteredOutPortfolioAccountIds,
    filteredOutAccountIds,
    setSelectedGroupAssetsByItem,
    selectedGroupAssetsByOption,
    setSelectedAssetFilter,
    selectedAssetFilterOption,
    setHidePortfolioSmallBalances,
    hideSmallBalances,
    setShowNetworkLogoOnNfts,
    showNetworkLogo,
    setHideUnownedNfts,
    hideUnownedNftsToggle,
    setGroupNftsByCollection,
    groupNftsByCollectionToggle,
    onSave,
    onClose
  ])

  // render
  return (
    <PopupModal
      onClose={onClose}
      title={
        showNftFilters
          ? getLocale('braveWalletPortfolioNftsFiltersTitle')
          : getLocale('braveWalletPortfolioFiltersTitle')
      }
      width='500px'
    >
      <ScrollableColumn>
        <ContentWrapper
          fullWidth={true}
          alignItems='flex-start'
        >
          {showNftFilters ? (
            <>
              <FilterToggleSection
                title={getLocale('braveWalletShowNetworkLogoOnNftsTitle')}
                description={getLocale(
                  'braveWalletShowNetworkLogoOnNftsDescription'
                )}
                icon='web3'
                isSelected={showNetworkLogo}
                setIsSelected={() => setShowNetworkLogo((prev) => !prev)}
              />

              <FilterToggleSection
                title={getLocale('braveWalletGroupByCollection')}
                description={getLocale(
                  'braveWalletPortfolioGroupByDescription'
                )}
                icon='stack'
                isSelected={groupNftsByCollectionToggle}
                setIsSelected={() =>
                  setGroupNftsByCollectionToggle((prev) => !prev)
                }
              />

              <FilterToggleSection
                title={getLocale('braveWalletHideNotOwnedNfTs')}
                description={''}
                icon='web3'
                isSelected={hideUnownedNftsToggle}
                setIsSelected={() => setHideUnownedNftsToggle((prev) => !prev)}
              />

              {/* Disabled until Spam NFTs feature is implemented in core */}
              {/* <FilterToggleSection
                title={getLocale('braveWalletShowSpamNftsTitle')}
                description={getLocale('braveWalletShowSpamNftsDescription')}
                icon='shield-star'
                isSelected={true}
                setIsSelected={
                  () => {}
                }
              /> */}
            </>
          ) : (
            <>
              <FilterDropdownSection
                title={getLocale('braveWalletPortfolioGroupByTitle')}
                description={getLocale(
                  'braveWalletPortfolioGroupByDescription'
                )}
                icon='stack'
                dropdownOptions={GroupAssetsByOptions}
                selectedOptionId={selectedGroupAssetsByOption}
                onSelectOption={setSelectedGroupAssetsByOption}
              />
              <FilterDropdownSection
                title={getLocale('braveWalletSortAssets')}
                description={getLocale('braveWalletSortAssetsDescription')}
                icon='sort-desc'
                dropdownOptions={AssetFilterOptions}
                selectedOptionId={selectedAssetFilterOption}
                onSelectOption={setSelectedAssetFilterOption}
              />

              <FilterToggleSection
                title={getLocale('braveWalletHideSmallBalances')}
                description={hideSmallBalancesDescription}
                icon='eye-on'
                isSelected={hideSmallBalances}
                setIsSelected={() => setHideSmallBalances((prev) => !prev)}
              />
            </>
          )}

          <VerticalDivider />
          <VerticalSpacer space={16} />

          <FilterNetworksSection
            filteredOutNetworkKeys={filteredOutNetworkKeys}
            setFilteredOutNetworkKeys={setFilteredOutNetworkKeys}
          />

          <VerticalDivider />
          <VerticalSpacer space={16} />

          <FilterAccountsSection
            filteredOutAccountIds={filteredOutAccountIds}
            setFilteredOutAccountIds={setFilteredOutAccountIds}
          />
        </ContentWrapper>
      </ScrollableColumn>

      <ButtonRow>
        <LeoSquaredButton
          onClick={onClose}
          kind='outline'
        >
          {getLocale('braveWalletButtonCancel')}
        </LeoSquaredButton>
        <HorizontalSpace space='16px' />
        <LeoSquaredButton onClick={onSaveChanges}>
          {getLocale('braveWalletButtonSaveChanges')}
        </LeoSquaredButton>
      </ButtonRow>
    </PopupModal>
  )
}
