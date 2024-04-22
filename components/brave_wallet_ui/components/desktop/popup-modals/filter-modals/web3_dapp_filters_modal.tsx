// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'
import { useLocalStorage } from '../../../../common/hooks/use_local_storage'
import { LOCAL_STORAGE_KEYS } from '../../../../common/constants/local-storage-keys'
import { makeInitialFilteredOutNetworkKeys } from '../../../../utils/local-storage-utils'

// components
import PopupModal from '..'
import { FilterNetworksSection } from './filter-components/filter-networks-section'

// styles
import {
  ScrollableColumn,
  VerticalSpacer,
  HorizontalSpace,
  LeoSquaredButton
} from '../../../shared/style'
import { ContentWrapper, ButtonRow } from './portfolio-filters-modal.style'
import { CategoryCheckboxes } from './filter-components/category-checkboxes'

interface Props {
  categories: string[]
  onClose: () => void
}

export const Web3DappFilters = ({ categories, onClose }: Props) => {
  // Local storage
  const [filteredOutDappNetworkKeys, setFilteredOutDappNetworkKeys] =
    useLocalStorage(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_DAPP_NETWORK_KEYS,
      makeInitialFilteredOutNetworkKeys
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

  // methods
  const isCategoryFilteredOut = React.useCallback(
    (category: string) => {
      return filteredOutCategories.includes(category)
    },
    [filteredOutCategories]
  )

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

  const onCheckCategory = React.useCallback(
    (category: string) => {
      if (isCategoryFilteredOut(category)) {
        setFilteredOutCategories(
          filteredOutCategories.filter((c) => c !== category)
        )
      } else {
        setFilteredOutCategories([...filteredOutCategories, category])
      }
    },
    [filteredOutCategories, isCategoryFilteredOut]
  )

  // computed
  const isSelectAll = React.useMemo(() => {
    return (
      filteredOutCategories.length > 0 &&
      categories.every((category) => filteredOutCategories.includes(category))
    )
  }, [categories, filteredOutCategories])

  const onSelectOrDeselectAllCategories = React.useCallback(() => {
    if (isSelectAll) {
      setFilteredOutCategories([])
      return
    }
    setFilteredOutCategories(categories)
  }, [categories, isSelectAll])

  return (
    <PopupModal
      onClose={onClose}
      title={getLocale('braveWalletPortfolioFiltersTitle')}
      width='500px'
      borderRadius={16}
    >
      <ScrollableColumn>
        <ContentWrapper
          fullWidth={true}
          alignItems='flex-start'
        >
          <CategoryCheckboxes
            title='Categories'
            marginBottom={16}
            onCheckCategory={onCheckCategory}
            isCategoryFilteredOut={isCategoryFilteredOut}
            categories={categories}
            isSelectAll={isSelectAll}
            onSelectOrDeselectAllCategories={onSelectOrDeselectAllCategories}
          />
          <VerticalSpacer space={16} />
          <FilterNetworksSection
            filteredOutNetworkKeys={filteredOutNetworkKeys}
            setFilteredOutNetworkKeys={setFilteredOutNetworkKeys}
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
