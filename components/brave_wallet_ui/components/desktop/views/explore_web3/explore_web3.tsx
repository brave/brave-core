// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import ProgressRing from '@brave/leo/react/progressRing'

// Types
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'
import {
  useLocalStorage, //
  useSyncedLocalStorage
} from '../../../../common/hooks/use_local_storage'
import {
  LOCAL_STORAGE_KEYS //
} from '../../../../common/constants/local-storage-keys'
import { capitalizeFirstLetter } from '../../../../utils/string-utils'
import { isDappMapEmpty } from '../../../../utils/dapp_utils'

// Hooks
import { useGetTopDappsQuery } from '../../../../common/slices/api.slice'
import {
  useGetDappRadarNetworks //
} from '../../../../common/slices/api.slice.extra'
import {
  getNetworkId //
} from '../../../../common/slices/entities/network.entity'
import { useQuery } from '../../../../common/hooks/use-query'

// Components
import {
  HeaderControlBar //
} from '../../../header_control_bar/header_control_bar'
import {
  Web3DappFilters //
} from '../../popup-modals/filter-modals/web3_dapp_filters_modal'
import { DappListItem } from './dapp_list_item'
import { DappFilter } from './dapp_filter'
import { DappDetails } from './web3_dapp_details'

// Styles
import { Column, Row, Text } from '../../../shared/style'
import { CategoryHeader, DappsGrid, PlainButton } from './explore_web3.style'

export const ExploreWeb3View = () => {
  // routing
  const history = useHistory()
  const query = useQuery()
  const selectedCategory = query.get('dappCategory')

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showFilters, setShowFilters] = React.useState<boolean>(false)
  const [selectedDapp, setSelectedDapp] = React.useState<
    BraveWallet.Dapp | undefined
  >(undefined)

  // local storage
  const [filteredOutNetworkKeys, setFilteredOutNetworkKeys] =
    useSyncedLocalStorage<string[]>(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_DAPP_NETWORK_KEYS,
      []
    )

  const [filteredOutCategories, setFilteredOutCategories] = useLocalStorage<
    string[]
  >(LOCAL_STORAGE_KEYS.FILTERED_OUT_DAPP_CATEGORIES, [])

  // queries
  const { dappNetworks } = useGetDappRadarNetworks()
  const { isFetching: isLoading, data: topDapps } = useGetTopDappsQuery()

  // memos
  const [visibleNetworks, visibleNetworkIds] = React.useMemo(() => {
    const networks =
      dappNetworks?.filter((network) => {
        return !filteredOutNetworkKeys.includes(getNetworkId(network))
      }) ?? []

    return [networks, networks.map(getNetworkId)]
  }, [dappNetworks, filteredOutNetworkKeys])

  const topDappsForChains = React.useMemo(() => {
    if (!topDapps) {
      return []
    }
    return topDapps.filter((dapp) => {
      return dapp.chains.some((chain) => visibleNetworkIds.includes(chain))
    })
  }, [topDapps, visibleNetworkIds])

  const controls = React.useMemo(() => {
    return [
      {
        buttonIconName: 'filter',
        onClick: () => setShowFilters(true)
      }
    ]
  }, [])

  // group dapps into categories
  const [dappCategories, categoryDappsMap, visibleCategories] =
    React.useMemo(() => {
      if (!topDappsForChains) {
        return [[], new Map<string, BraveWallet.Dapp[]>(), []]
      }

      const categoriesSet = new Set<string>()
      const categoriesMap = new Map<string, BraveWallet.Dapp[]>()

      topDappsForChains.forEach((dapp) => {
        dapp.categories.forEach((category) => {
          categoriesSet.add(category)
          if (!categoriesMap.has(category)) {
            categoriesMap.set(category, [])
          }
          categoriesMap.get(category)!.push(dapp)
        })
      })

      const categoriesList = Array.from(categoriesSet)
      const visibleCategories = categoriesList.filter(
        (category) => !filteredOutCategories.includes(category)
      )
      return [categoriesList, categoriesMap, visibleCategories]
    }, [filteredOutCategories, topDappsForChains])

  const visibleDappsMap = React.useMemo(() => {
    if (isDappMapEmpty(categoryDappsMap)) {
      return categoryDappsMap
    }
    const searchValueLower = searchValue.toLowerCase().trim()

    const filterResultsMap = new Map<string, BraveWallet.Dapp[]>()
    categoryDappsMap.forEach((categoryDapps, category) => {
      if (!filteredOutCategories.includes(category)) {
        // filter items based on network and search term
        const categoryVisibleDapps = categoryDapps.filter((dapp) => {
          return (
            dapp.name.toLowerCase().includes(searchValueLower) ||
            dapp.description.toLowerCase().includes(searchValueLower)
          )
        })
        if (categoryVisibleDapps.length > 0) {
          filterResultsMap.set(category, categoryVisibleDapps)
        }
      }
    })

    return filterResultsMap
  }, [categoryDappsMap, filteredOutCategories, searchValue])

  const selectedCategoryDapps = React.useMemo(() => {
    return selectedCategory ? visibleDappsMap.get(selectedCategory) : []
  }, [selectedCategory, visibleDappsMap])

  // computed
  const showFiltersRow =
    (visibleCategories.length < dappCategories.length ||
      visibleNetworks.length < dappNetworks.length) &&
    selectedCategory === null

  // methods
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
      if (visibleCategories.length === 1) {
        setFilteredOutCategories([]) // clear filter
        return
      }
      setFilteredOutCategories((prev) => prev.concat(category))
    },
    [setFilteredOutCategories, visibleCategories.length]
  )

  const onRemoveNetworkFilter = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      if (visibleNetworks.length === 1) {
        setFilteredOutNetworkKeys([]) // clear filter
        return
      }
      setFilteredOutNetworkKeys((prev) => prev.concat(getNetworkId(network)))
    },
    [setFilteredOutNetworkKeys, visibleNetworks.length]
  )

  const onClearFilters = React.useCallback(() => {
    setFilteredOutNetworkKeys([])
    setFilteredOutCategories([])
  }, [setFilteredOutCategories, setFilteredOutNetworkKeys])

  // render
  if (isLoading || !topDappsForChains) {
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
        padding='10px 20px'
      >
        <HeaderControlBar
          actions={controls}
          onSearchValueChange={setSearchValue}
          searchValue={searchValue}
          title={
            !selectedCategory
              ? getLocale('braveWalletWeb3')
              : capitalizeFirstLetter(selectedCategory)
          }
          onClickBackButton={selectedCategory ? onCategoryBack : undefined}
        />

        {showFiltersRow && (
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
              {filteredOutCategories.length > 0
                ? visibleCategories.map((category) => (
                    <DappFilter
                      key={category}
                      label={category}
                      onClick={() => onRemoveCategoryFilter(category)}
                    />
                  ))
                : null}

              {filteredOutNetworkKeys.length > 0
                ? visibleNetworks.map((network) => (
                    <DappFilter
                      key={network.chainId}
                      label={network.chainName}
                      onClick={() => onRemoveNetworkFilter(network)}
                    />
                  ))
                : null}
            </Row>
            <PlainButton onClick={onClearFilters}>
              {getLocale('braveWalletClearFilters')}
            </PlainButton>
          </Row>
        )}

        {!isDappMapEmpty(visibleDappsMap) ||
        selectedCategoryDapps?.length === 0 ? (
          selectedCategory ? (
            <DappsGrid>
              {(selectedCategoryDapps || []).map((dapp) => (
                <DappListItem
                  key={dapp.id}
                  dapp={dapp}
                  onClick={() => setSelectedDapp(dapp)}
                />
              ))}
            </DappsGrid>
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
                        onClick={() => setSelectedDapp(dapp)}
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

      {selectedDapp ? (
        <DappDetails
          isOpen
          dapp={selectedDapp}
          onClose={() => {
            setSelectedDapp(undefined)
          }}
        />
      ) : null}

      {showFilters && (
        <Web3DappFilters
          categories={dappCategories}
          onClose={() => setShowFilters(false)}
        />
      )}
    </>
  )
}
