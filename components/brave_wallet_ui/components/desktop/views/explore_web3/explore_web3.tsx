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
import { useLocalStorage } from '../../../../common/hooks/use_local_storage'
import { LOCAL_STORAGE_KEYS } from '../../../../common/constants/local-storage-keys'

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

// Styles
import { Column, Row } from '../../../shared/style'
import { ControlsRow } from '../portfolio/style'
import { BraveWallet } from '../../../../constants/types'
import { getDappNetworkIds, isMapEmpty } from '../../../../utils/dapp_utils'
import { CategoryHeader, DappsGrid, ShowMore } from './explore_web3.style'

export const ExploreWeb3View = () => {
  // routing
  const history = useHistory()

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showFilters, setShowFilters] = React.useState<boolean>(false)

  // local storage
  const [filteredOutCategories] = useLocalStorage<string[]>(
    LOCAL_STORAGE_KEYS.FILTERED_OUT_DAPP_CATEGORIES,
    []
  )
  const [filteredOutNetworkKeys] = useLocalStorage<string[]>(
    LOCAL_STORAGE_KEYS.FILTERED_OUT_DAPP_NETWORK_KEYS,
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

  const [visibleNetworks, visibleNetworkIds] = React.useMemo(() => {
    const visibleNetworks = networks.filter(
      (network) =>
        !filteredOutNetworkKeys.includes(
          networkEntityAdapter.selectId(network).toString()
        )
    )
    return [visibleNetworks, visibleNetworks.map(networkEntityAdapter.selectId)]
  }, [networks, filteredOutNetworkKeys])

  const visibleDappsMap = React.useMemo(() => {
    const filterResultsMap = new Map<string, BraveWallet.Dapp[]>()

    if (isMapEmpty(categoryDappsMap)) {
      return filterResultsMap
    }

    categoryDappsMap.forEach((dapps, category) => {
      const categorySearchResults = dapps.filter((dapp) => {
        const dappNetworkIds = getDappNetworkIds(dapp.chains, visibleNetworks)
        return (
          !filteredOutCategories.some((category) =>
            dapp.categories.includes(category)
          ) ||
          dappNetworkIds.some((networkId) =>
            visibleNetworkIds.includes(networkId.toString())
          )
        )
      })

      if (categorySearchResults.length > 0) {
        filterResultsMap.set(category, categorySearchResults)
      }
    })

    return filterResultsMap
  }, [
    categoryDappsMap,
    filteredOutCategories,
    visibleNetworkIds,
    visibleNetworks
  ])

  const searchedDappsMap = React.useMemo(() => {
    const searchResultsMap = new Map<string, BraveWallet.Dapp[]>()

    if (isMapEmpty(visibleDappsMap)) {
      return searchResultsMap
    }

    const searchValueLower = searchValue.toLowerCase().trim()

    if (!searchValueLower) {
      return categoryDappsMap
    }

    categoryDappsMap.forEach((dapps, category) => {
      const categorySearchResults = dapps.filter(
        (dapp) =>
          dapp.name.toLowerCase().includes(searchValueLower) ||
          dapp.description.toLowerCase().includes(searchValueLower)
      )

      if (categorySearchResults.length > 0) {
        searchResultsMap.set(category, categorySearchResults)
      }
    })

    return searchResultsMap
  }, [categoryDappsMap, searchValue, visibleDappsMap])

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
          title={getLocale('braveWalletWeb3')}
        />

        {!isMapEmpty(searchedDappsMap) ? (
          <DappsGrid>
            {Array.from(searchedDappsMap).map(([category, dapps]) => (
              <Column
                key={category}
                width='100%'
                margin='0 0 32px 0'
              >
                <CategoryHeader>{category}</CategoryHeader>
                <Column width='100%'>
                  {dapps.slice(0, 3).map((dapp) => (
                    <DappListItem
                      key={dapp.id}
                      dapp={dapp}
                      onClick={() =>
                        history.push(makeDappDetailsRoute(dapp.id.toString()))
                      }
                    />
                  ))}
                </Column>
                <Row justifyContent='center'>
                  <ShowMore>Show more</ShowMore>
                </Row>
              </Column>
            ))}
          </DappsGrid>
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
