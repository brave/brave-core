// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'
import { useLocalStorage } from '../../../../common/hooks/use_local_storage'
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../common/constants/local-storage-keys'
import {
  useGetDappRadarNetworks //
} from '../../../../common/slices/api.slice.extra'

// components
import PopupModal from '..'
import {
  FilterNetworksSection //
} from './filter-components/filter-networks-section'
import { CategoryCheckboxes } from './filter-components/category-checkboxes'

// styles
import {
  ScrollableColumn,
  HorizontalSpace,
  LeoSquaredButton
} from '../../../shared/style'
import { ContentWrapper, ButtonRow } from './portfolio-filters-modal.style'

interface Props {
  categories: string[]
  onClose: () => void
}

export const Web3DappFilters = ({ categories, onClose }: Props) => {
  // Local storage
  const [filteredOutDappNetworkKeys, setFilteredOutDappNetworkKeys] =
    useLocalStorage<string[]>(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_DAPP_NETWORK_KEYS,
      []
    )

  const [filteredOutDappCategories, setFilteredOutDappCategories] =
    useLocalStorage<string[]>(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_DAPP_CATEGORIES,
      []
    )

  // state
  const [filteredOutNetworkKeys, setFilteredOutNetworkKeys] = React.useState<
    string[]
  >(filteredOutDappNetworkKeys)

  const [filteredOutCategories, setFilteredOutCategories] = React.useState<
    string[]
  >(filteredOutDappCategories)

  // queries
  const { dappNetworks } = useGetDappRadarNetworks()

  // methods
  const isCategoryFilteredOut = (category: string) =>
    filteredOutCategories.includes(category)

  const onSaveChanges = React.useCallback(() => {
    setFilteredOutDappNetworkKeys(filteredOutNetworkKeys)
    setFilteredOutDappCategories(filteredOutCategories)
    onClose()
  }, [
    setFilteredOutDappNetworkKeys,
    filteredOutNetworkKeys,
    setFilteredOutDappCategories,
    filteredOutCategories,
    onClose
  ])

  const onCheckCategory = (category: string) =>
    setFilteredOutCategories((prev) =>
      prev.includes(category)
        ? prev.filter((c) => c !== category)
        : prev.concat(category)
    )

  // computed
  const showSelectAll =
    filteredOutCategories.length > 0 &&
    categories.some((category) => filteredOutCategories.includes(category))

  const onSelectOrDeselectAllCategories = React.useCallback(() => {
    setFilteredOutCategories(showSelectAll ? [] : categories)
  }, [categories, showSelectAll])

  return (
    <PopupModal
      onClose={onClose}
      title={getLocale('braveWalletPortfolioFiltersTitle')}
      width='500px'
    >
      <ScrollableColumn>
        <ContentWrapper
          fullWidth={true}
          alignItems='flex-start'
          gap={16}
        >
          <CategoryCheckboxes
            title='Categories'
            marginBottom={16}
            onCheckCategory={onCheckCategory}
            isCategoryFilteredOut={isCategoryFilteredOut}
            categories={categories}
            isSelectAll={showSelectAll}
            onSelectOrDeselectAllCategories={onSelectOrDeselectAllCategories}
          />
          <FilterNetworksSection
            filteredOutNetworkKeys={filteredOutNetworkKeys}
            setFilteredOutNetworkKeys={setFilteredOutNetworkKeys}
            networksSubset={dappNetworks}
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
