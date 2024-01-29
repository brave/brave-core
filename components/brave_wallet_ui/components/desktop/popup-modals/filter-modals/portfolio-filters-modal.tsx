// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import { useDispatch } from 'react-redux'
import { useLocation } from 'react-router'

// Types
import { WalletRoutes } from '../../../../constants/types'

// Options
import { AssetFilterOptions } from '../../../../options/asset-filter-options'
import {
  GroupAssetsByOptions //
} from '../../../../options/group-assets-by-options'

// Selectors
import {
  useUnsafeWalletSelector,
  useSafeWalletSelector
} from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'

// Actions
import { WalletActions } from '../../../../common/actions'

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
  HorizontalSpace
} from '../../../shared/style'
import { ContentWrapper, ButtonRow } from './portfolio-filters-modal.style'

interface Props {
  onClose: () => void
}

export const PortfolioFiltersModal = (props: Props) => {
  const { onClose } = props

  // routing
  const { pathname: currentRoute } = useLocation()

  // Redux
  const dispatch = useDispatch()

  // queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

  // Selectors
  const filteredOutPortfolioNetworkKeys = useUnsafeWalletSelector(
    WalletSelectors.filteredOutPortfolioNetworkKeys
  )
  const filteredOutPortfolioAccountIds = useUnsafeWalletSelector(
    WalletSelectors.filteredOutPortfolioAccountIds
  )
  const selectedAssetFilter = useSafeWalletSelector(
    WalletSelectors.selectedAssetFilter
  )
  const selectedGroupAssetsByItem = useSafeWalletSelector(
    WalletSelectors.selectedGroupAssetsByItem
  )
  const hidePortfolioSmallBalances = useSafeWalletSelector(
    WalletSelectors.hidePortfolioSmallBalances
  )
  const showNetworkLogoOnNfts = useSafeWalletSelector(
    WalletSelectors.showNetworkLogoOnNfts
  )

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

  const onUpdateSelectedGroupAssetsByOption = React.useCallback(() => {
    // Update Selected Group Assets By Option in Local Storage
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_ASSETS_BY,
      selectedGroupAssetsByOption
    )
    // Update Selected Group Assets By Option in Redux
    dispatch(
      WalletActions.setSelectedGroupAssetsByItem(selectedGroupAssetsByOption)
    )
  }, [selectedGroupAssetsByOption])

  const onUpdateSelectedAssetFilterOption = React.useCallback(() => {
    // Update Selected Asset Filter Option in Local Storage
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.PORTFOLIO_ASSET_FILTER_OPTION,
      selectedAssetFilterOption
    )
    // Update Selected Asset Filter Option in Redux
    dispatch(
      WalletActions.setSelectedAssetFilterItem(selectedAssetFilterOption)
    )
  }, [selectedAssetFilterOption])

  const onUpdateFilteredOutNetworkKeys = React.useCallback(() => {
    // Update Filtered Out Network Keys in Local Storage
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_PORTFOLIO_NETWORK_KEYS,
      JSON.stringify(filteredOutNetworkKeys)
    )

    // Update Filtered Out Network Keys in Redux
    dispatch(
      WalletActions.setFilteredOutPortfolioNetworkKeys(filteredOutNetworkKeys)
    )
  }, [filteredOutNetworkKeys])

  const onUpdateFilteredOutAccountIds = React.useCallback(() => {
    // Update Filtered Out Account Ids in Local Storage
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_PORTFOLIO_ACCOUNT_IDS,
      JSON.stringify(filteredOutAccountIds)
    )

    // Update Filtered Out Account Ids in Redux
    dispatch(
      WalletActions.setFilteredOutPortfolioAccountIds(filteredOutAccountIds)
    )
  }, [filteredOutAccountIds])

  const onUpdateHidePortfolioSmallBalances = React.useCallback(() => {
    // Update Hide Small Portfolio Balances in Local Storage
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_SMALL_BALANCES,
      JSON.stringify(hideSmallBalances)
    )

    // Update Hide Small Portfolio Balances in Redux
    dispatch(WalletActions.setHidePortfolioSmallBalances(hideSmallBalances))
  }, [hideSmallBalances])

  const hideSmallBalancesDescription = React.useMemo(() => {
    const minAmount = new Amount(HIDE_SMALL_BALANCES_FIAT_THRESHOLD)
      .formatAsFiat(defaultFiatCurrency)
      .split('.')[0]
    return getLocale('braveWalletHideSmallBalancesDescription').replace(
      '$1',
      minAmount
    )
  }, [defaultFiatCurrency])

  const showNftFilters = React.useMemo(() => {
    return currentRoute === WalletRoutes.PortfolioNFTs
  }, [currentRoute])

  const onUpdateShowNetworkLogoOnNfts = React.useCallback(() => {
    // Update Show Network Logo on NFTs in Local Storage
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.SHOW_NETWORK_LOGO_ON_NFTS,
      JSON.stringify(showNetworkLogo)
    )

    // Update Show Network Logo on NFTs in Redux
    dispatch(WalletActions.setShowNetworkLogoOnNfts(showNetworkLogo))
  }, [showNetworkLogo])

  // Methods
  const onSaveChanges = React.useCallback(() => {
    onUpdateSelectedGroupAssetsByOption()
    onUpdateSelectedAssetFilterOption()
    onUpdateFilteredOutNetworkKeys()
    onUpdateFilteredOutAccountIds()
    onUpdateHidePortfolioSmallBalances()
    onUpdateShowNetworkLogoOnNfts()
    onClose()
  }, [
    onUpdateSelectedGroupAssetsByOption,
    onUpdateSelectedAssetFilterOption,
    onUpdateFilteredOutNetworkKeys,
    onUpdateFilteredOutAccountIds,
    onUpdateHidePortfolioSmallBalances,
    onUpdateShowNetworkLogoOnNfts,
    onClose
  ])

  return (
    <PopupModal
      onClose={onClose}
      title={
        showNftFilters
          ? getLocale('braveWalletPortfolioNftsFiltersTitle')
          : getLocale('braveWalletPortfolioFiltersTitle')
      }
      width='500px'
      borderRadius={16}
    >
      <ScrollableColumn>
        <ContentWrapper
          fullWidth={true}
          alignItems='flex-start'
        >
          {showNftFilters && (
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
          )}

          <FilterDropdownSection
            title={getLocale('braveWalletPortfolioGroupByTitle')}
            description={getLocale('braveWalletPortfolioGroupByDescription')}
            icon='stack'
            dropdownOptions={GroupAssetsByOptions}
            selectedOptionId={selectedGroupAssetsByOption}
            onSelectOption={setSelectedGroupAssetsByOption}
          />

          {!showNftFilters && (
            <>
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
        <Button
          onClick={onClose}
          kind='outline'
        >
          {getLocale('braveWalletButtonCancel')}
        </Button>
        <HorizontalSpace space='16px' />
        <Button onClick={onSaveChanges}>
          {getLocale('braveWalletButtonSaveChanges')}
        </Button>
      </ButtonRow>
    </PopupModal>
  )
}
