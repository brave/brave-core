// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

import ProgressRing from '@brave/leo/react/progressRing'

// Constants & Options
import { ExploreNavOptions } from '../../../../options/nav-options'

// Utils
import { getLocale } from '../../../../../common/locale'
import { makeDappDetailsRoute } from '../../../../utils/routes-utils'
import { useSyncedLocalStorage } from '../../../../common/hooks/use_local_storage'
import { LOCAL_STORAGE_KEYS } from '../../../../common/constants/local-storage-keys'
import { capitalizeFirstLetter } from '../../../../utils/string-utils'
import { makeInitialFilteredOutNetworkKeys } from '../../../../utils/local-storage-utils'
import { getDappNetworkIds, isDappMapEmpty } from '../../../../utils/dapp_utils'

// Hooks
import {
  useGetTopDappsQuery,
  useGetVisibleNetworksQuery
} from '../../../../common/slices/api.slice'
import {
  networkEntityAdapter //
} from '../../../../common/slices/entities/network.entity'

// Components
import {
  SegmentedControl //
} from '../../../shared/segmented_control/segmented_control'
import {
  HeaderControlBar //
} from '../../../header_control_bar/header_control_bar'
import { Web3DappFilters } from '../../popup-modals/filter-modals/web3_dapp_filters_modal'
import { DappListItem } from './dapp_list_item'
import { VirtualizedDappsList } from './virtualized_dapps_list'

// Styles
import { Column, Row, Text } from '../../../shared/style'
import { ControlsRow } from '../portfolio/style'
import { BraveWallet, WalletRoutes } from '../../../../constants/types'
import {
  CategoryHeader,
  DappsGrid,
  FilterLabel,
  PlainButton,
  FilterClose
} from './explore_web3.style'

export const ExploreWeb3View = () => {
  // routing
  const history = useHistory()
  const params = new URLSearchParams(history.location.search)
  const selectedCategory = params.get('dappCategory')

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showFilters, setShowFilters] = React.useState<boolean>(false)

  // local storage
  const [filteredOutNetworkKeys, setFilteredOutNetworkKeys] =
    useSyncedLocalStorage(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_DAPP_NETWORK_KEYS,
      makeInitialFilteredOutNetworkKeys
    )

  const [filteredOutCategories, setFilteredOutCategories] =
    useSyncedLocalStorage<string[]>(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_DAPP_CATEGORIES,
      []
    )

  // queries
  const { isLoading, data: topDapps } = useGetTopDappsQuery(undefined)
  const { data: networks } = useGetVisibleNetworksQuery()

  // memos
  const controls = React.useMemo(() => {
    return [
      {
        buttonIconName: 'funnel',
        onClick: () => setShowFilters(true)
      }
    ]
  }, [])

  // group dapps into categories
  const [dappCategories, categoryDappsMap] = React.useMemo(() => {
    if (!topDapps) return [[], new Map<string, BraveWallet.Dapp[]>()]

    const categoriesSet = new Set<string>()
    const categoriesMap = new Map<string, BraveWallet.Dapp[]>()

    topDapps.forEach((dapp) => {
      dapp.categories.forEach((category) => {
        categoriesSet.add(category)
        if (!categoriesMap.has(category)) {
          categoriesMap.set(category, [])
        }
        categoriesMap.get(category)!.push(dapp)
      })
    })

    const categoriesList = Array.from(categoriesSet)
    return [categoriesList, categoriesMap]
  }, [topDapps])

  const [visibleNetworks, visibleNetworkIds, filteredOutNetworks] =
    React.useMemo(() => {
      const visibleNetworks: BraveWallet.NetworkInfo[] = []
      const filteredOutNetworks: BraveWallet.NetworkInfo[] = []

      networks.forEach((network) => {
        if (
          filteredOutNetworkKeys.includes(
            networkEntityAdapter.selectId(network).toString()
          )
        ) {
          filteredOutNetworks.push(network)
        } else {
          visibleNetworks.push(network)
        }
      })

      return [
        visibleNetworks,
        visibleNetworks.map(networkEntityAdapter.selectId),
        filteredOutNetworks
      ]
    }, [networks, filteredOutNetworkKeys])

  const filterVisibleDapps = React.useCallback(
    (dapps: BraveWallet.Dapp[], searchTerm: string) => {
      return dapps.filter((dapp) => {
        const dappNetworkIds = getDappNetworkIds(dapp.chains, visibleNetworks)
        return (
          dappNetworkIds.some((networkId) =>
            visibleNetworkIds.includes(networkId.toString())
          ) &&
          (dapp.name.toLowerCase().includes(searchTerm) ||
            dapp.description.toLowerCase().includes(searchTerm))
        )
      })
    },
    [visibleNetworkIds, visibleNetworks]
  )

  const visibleDappsMap = React.useMemo(() => {
    if (isDappMapEmpty(categoryDappsMap)) {
      return categoryDappsMap
    }
    const searchValueLower = searchValue.toLowerCase().trim()

    const filterResultsMap = new Map<string, BraveWallet.Dapp[]>()
    categoryDappsMap.forEach((categoryDapps, category) => {
      if (!filteredOutCategories.includes(category)) {
        // filter items based on network and search term
        const categoryVisibleDapps = filterVisibleDapps(
          categoryDapps,
          searchValueLower
        )
        if (categoryVisibleDapps.length > 0) {
          filterResultsMap.set(category, categoryVisibleDapps)
        }
      }
    })

    return filterResultsMap
  }, [categoryDappsMap, filterVisibleDapps, filteredOutCategories, searchValue])

  const selectedCategoryDapps = React.useMemo(() => {
    return selectedCategory ? visibleDappsMap.get(selectedCategory) : []
  }, [selectedCategory, visibleDappsMap])

  // methods
  const onDappClick = React.useCallback(
    (dappId: number) => {
      history.push(makeDappDetailsRoute(dappId.toString()))
    },
    [history]
  )

  const onSelectCategory = React.useCallback(
    (category: string) => {
      history.push({
        pathname: WalletRoutes.Web3,
        search: `?dappCategory=${category}`
      })
    },
    [history]
  )

  const onCategoryBack = React.useCallback(() => {
    history.push({
      pathname: WalletRoutes.Web3,
      search: ''
    })
  }, [history])

  const onRemoveCategoryFilter = React.useCallback(
    (category: string) => {
      const updatedCategories = filteredOutCategories.filter(
        (c) => c !== category
      )
      setFilteredOutCategories(updatedCategories)
    },
    [filteredOutCategories, setFilteredOutCategories]
  )

  const onRemoveNetworkFilter = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      const updatedNetworksKeys = filteredOutNetworkKeys.filter(
        (key) => key !== networkEntityAdapter.selectId(network).toString()
      )
      setFilteredOutNetworkKeys(updatedNetworksKeys)
    },
    [filteredOutNetworkKeys, setFilteredOutNetworkKeys]
  )

  const onClearFilters = React.useCallback(() => {
    setFilteredOutNetworkKeys([])
    setFilteredOutCategories([])
  }, [setFilteredOutCategories, setFilteredOutNetworkKeys])

  // render
  if (isLoading || !topDapps) {
    return (
      <Column
        fullHeight
        fullWidth
      >
        <ProgressRing />
      </Column>
    )
  }

  return (
    <>
      <Column
        fullHeight
        fullWidth
        justifyContent='flex-start'
      >
        <ControlsRow>
          <SegmentedControl
            navOptions={ExploreNavOptions}
            width={384}
          />
        </ControlsRow>
        <HeaderControlBar
          actions={controls}
          onSearchValueChange={setSearchValue}
          searchValue={searchValue}
          title={
            !selectedCategory
              ? getLocale('braveWalletWeb3')
              : capitalizeFirstLetter(selectedCategory)
          }
          showBackButton={selectedCategory !== null}
          onClickBackButton={onCategoryBack}
        />

        {(filteredOutCategories.length > 0 ||
          filteredOutNetworks.length > 0) && (
          <Row
            justifyContent='flex-start'
            alignItems='center'
            gap='16px'
            marginBottom='32px'
          >
            <Text
              textAlign='left'
              isBold
              textSize='14px'
            >
              {getLocale('braveWalletFilters')}:
            </Text>
            <Row
              gap='8px'
              justifyContent='flex-start'
              flexWrap='wrap'
              flex='0 1 auto'
            >
              {filteredOutCategories.map((category) => (
                <FilterLabel key={category}>
                  {category}
                  <div
                    slot='icon-after'
                    onClick={() => onRemoveCategoryFilter(category)}
                  >
                    <FilterClose />
                  </div>
                </FilterLabel>
              ))}

              {filteredOutNetworks.map((network) => (
                <FilterLabel key={network.chainId}>
                  {network.chainName}
                  <div
                    slot='icon-after'
                    onClick={() => onRemoveNetworkFilter(network)}
                  >
                    <FilterClose />
                  </div>
                </FilterLabel>
              ))}
            </Row>
            <PlainButton onClick={onClearFilters}>
              {getLocale('braveWalletClearFilters')}
            </PlainButton>
          </Row>
        )}

        {!isDappMapEmpty(visibleDappsMap) ||
        selectedCategoryDapps?.length === 0 ? (
          selectedCategory ? (
            <VirtualizedDappsList
              dappsList={selectedCategoryDapps || []}
              onClickDapp={onDappClick}
            />
          ) : (
            <DappsGrid>
              {Array.from(visibleDappsMap).map(([category, dapps]) => (
                <Column
                  key={category}
                  width='100%'
                  margin='0 0 18px 0'
                >
                  <CategoryHeader>{category}</CategoryHeader>
                  <Column width='100%'>
                    {dapps.slice(0, 3).map((dapp) => (
                      <DappListItem
                        key={dapp.id}
                        dapp={dapp}
                        onClick={() => onDappClick(dapp.id)}
                      />
                    ))}
                  </Column>
                  <Row justifyContent='center'>
                    <PlainButton onClick={() => onSelectCategory(category)}>
                      {getLocale('braveWalletShowMore')}
                    </PlainButton>
                  </Row>
                </Column>
              ))}
            </DappsGrid>
          )
        ) : (
          <Row>
            <h2>{getLocale('braveWalletNoDappsFound')}</h2>
          </Row>
        )}
      </Column>

      {showFilters && (
        <Web3DappFilters
          categories={dappCategories}
          onClose={() => setShowFilters(false)}
        />
      )}
    </>
  )
}
