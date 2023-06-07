// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import { useDispatch } from 'react-redux'

// Options
import {
  AssetFilterOptions
} from '../../../../options/asset-filter-options'
import {
  GroupAssetsByOptions
} from '../../../../options/group-assets-by-options'

// Selectors
import {
  useUnsafeWalletSelector,
  useSafeWalletSelector
} from '../../../../common/hooks/use-safe-selector'
import {
  WalletSelectors
} from '../../../../common/selectors'

// Actions
import {
  WalletActions
} from '../../../../common/actions'

// Constants
import {
  LOCAL_STORAGE_KEYS
} from '../../../../common/constants/local-storage-keys'
import {
  HIDE_SMALL_BALANCES_FIAT_THRESHOLD
} from '../../../../common/constants/magics'

// Hooks
import {
  useOnClickOutside
} from '../../../../common/hooks/useOnClickOutside'
import { useLib } from '../../../../common/hooks/useLib'

// Utils
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'

// Components
import { PopupModal } from '../..'
import {
  FilterNetworksSection
} from './filter-components/filter-networks-section'
import {
  FilterAccountsSection
} from './filter-components/filter-accounts-section'
import {
  FilterToggleSection
} from './filter-components/filter-toggle-section'
import {
  FilterDropdownSection
} from './filter-components/filter-dropdown-section'

// Styles
import {
  Row,
  Column,
  VerticalDivider,
  VerticalSpacer,
  ScrollableColumn,
  HorizontalSpace
} from '../../../shared/style'


interface Props {
  onClose: () => void
}

export const PortfolioFiltersModal = (props: Props) => {
  const { onClose } = props

  // Redux
  const dispatch = useDispatch()

  // Selectors
  const filteredOutPortfolioNetworkKeys =
    useUnsafeWalletSelector(WalletSelectors.filteredOutPortfolioNetworkKeys)
  const filteredOutPortfolioAccountAddresses =
    useUnsafeWalletSelector(
      WalletSelectors.filteredOutPortfolioAccountAddresses
    )
  const selectedAssetFilter =
    useSafeWalletSelector(WalletSelectors.selectedAssetFilter)
  const selectedGroupAssetsByItem =
    useSafeWalletSelector(WalletSelectors.selectedGroupAssetsByItem)
  const hidePortfolioSmallBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioSmallBalances)
  const defaultCurrencies =
    useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)
  const selectedPortfolioTimeline =
    useSafeWalletSelector(WalletSelectors.selectedPortfolioTimeline)

  // State
  const [filteredOutNetworkKeys, setFilteredOutNetworkKeys] =
    React.useState<string[]>(filteredOutPortfolioNetworkKeys)
  const [filteredOutAccountAddresses, setFilteredOutAccountAddresses] =
    React.useState<string[]>(filteredOutPortfolioAccountAddresses)
  const [hideSmallBalances, setHideSmallBalances] =
    React.useState<boolean>(hidePortfolioSmallBalances)
  const [selectedAssetFilterOption, setSelectedAssetFilterOption] =
    React.useState<string>(selectedAssetFilter)
  const [selectedGroupAssetsByOption, setSelectedGroupAssetsByOption] =
    React.useState<string>(selectedGroupAssetsByItem)

  // refs
  const portfolioFiltersRef =
    React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(
    portfolioFiltersRef,
    () => onClose(),
    true
  )

  const { refreshTokenPriceHistory } = useLib()

  const onUpdateSelectedGroupAssetsByOption = React.useCallback(() => {
    // Update Selected Group Assets By Option in Local Storage
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_ASSETS_BY,
      selectedGroupAssetsByOption
    )
    // Update Selected Group Assets By Option in Redux
    dispatch(
      WalletActions
        .setSelectedGroupAssetsByItem(selectedGroupAssetsByOption)
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
      WalletActions
        .setSelectedAssetFilterItem(selectedAssetFilterOption)
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
      WalletActions
        .setFilteredOutPortfolioNetworkKeys(
          filteredOutNetworkKeys
        ))
  }, [filteredOutNetworkKeys])


  const onUpdateFilteredOutAccountAddresses = React.useCallback(() => {
    // Update Filtered Out Account Addresses in Local Storage
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_PORTFOLIO_ACCOUNT_ADDRESSES,
      JSON.stringify(filteredOutAccountAddresses)
    )

    // Update Filtered Out Account Addresses in Redux
    dispatch(
      WalletActions
        .setFilteredOutPortfolioAccountAddresses(
          filteredOutAccountAddresses
        ))
  }, [filteredOutAccountAddresses])


  const onUpdateHidePortfolioSmallBalances = React.useCallback(() => {
    // Update Hide Small Portfolio Balances in Local Storage
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_SMALL_BALANCES,
      JSON.stringify(hideSmallBalances)
    )

    // Update Hide Small Portfolio Balances in Redux
    dispatch(
      WalletActions
        .setHidePortfolioSmallBalances(
          hideSmallBalances
        ))
  }, [hideSmallBalances])

  const hideSmallBalancesDescription = React.useMemo(() => {
    const minAmount =
      new Amount(HIDE_SMALL_BALANCES_FIAT_THRESHOLD)
        .formatAsFiat(defaultCurrencies.fiat)
        .split('.')[0]
    return getLocale('braveWalletHideSmallBalancesDescription')
      .replace('$1', minAmount)
  }, [defaultCurrencies.fiat])

  // Methods
  const onSaveChanges = React.useCallback(() => {
    onUpdateSelectedGroupAssetsByOption()
    onUpdateSelectedAssetFilterOption()
    onUpdateFilteredOutNetworkKeys()
    onUpdateFilteredOutAccountAddresses()
    onUpdateHidePortfolioSmallBalances()
    dispatch(WalletActions.setIsFetchingPortfolioPriceHistory(true))
    dispatch(refreshTokenPriceHistory(selectedPortfolioTimeline))
    onClose()
  }, [
    onUpdateSelectedGroupAssetsByOption,
    onUpdateSelectedAssetFilterOption,
    onUpdateFilteredOutNetworkKeys,
    onUpdateFilteredOutAccountAddresses,
    onUpdateHidePortfolioSmallBalances,
    refreshTokenPriceHistory,
    selectedPortfolioTimeline,
    onClose
  ])

  return (
    <PopupModal
      onClose={onClose}
      title={getLocale('braveWalletPortfolioFiltersTitle')}
      width='500px'
      borderRadius={16}
      ref={portfolioFiltersRef}
    >
      <ScrollableColumn>
        <Column
          padding='16px 32px'
          fullWidth={true}
          alignItems='flex-start'
        >

          <FilterDropdownSection
            title={getLocale('braveWalletPortfolioGroupByTitle')}
            description={getLocale('braveWalletPortfolioGroupByDescription')}
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
            setIsSelected={
              () => setHideSmallBalances(prev => !prev)
            }
          />

          <VerticalDivider />
          <VerticalSpacer space={16} />

          <FilterNetworksSection
            filteredOutNetworkKeys={filteredOutNetworkKeys}
            setFilteredOutNetworkKeys={setFilteredOutNetworkKeys}
          />

          <VerticalDivider />
          <VerticalSpacer space={16} />

          <FilterAccountsSection
            filteredOutAccountAddresses={filteredOutAccountAddresses}
            setFilteredOutAccountAddresses={setFilteredOutAccountAddresses}
          />

        </Column>
      </ScrollableColumn>

      <Row
        padding={32}
      >
        <Button
          onClick={onClose}
          kind='outline'
        >
          {getLocale('braveWalletButtonCancel')}
        </Button>
        <HorizontalSpace space='16px' />
        <Button
          onClick={onSaveChanges}
        >
          {getLocale('braveWalletButtonSaveChanges')}
        </Button>
      </Row>

    </PopupModal>
  )
}
