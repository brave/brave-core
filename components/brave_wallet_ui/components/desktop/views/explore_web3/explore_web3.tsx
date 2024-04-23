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
import { DividerLine } from '../../../extension/divider'
import { Web3DappFilters } from '../../popup-modals/filter-modals/web3_dapp_filters_modal'

// Styles
import { Column, Row } from '../../../shared/style'
import { ControlsRow } from '../portfolio/style'
import { VirtualizedDappsList } from './virtualized_dapps_list'
import { BraveWallet } from '../../../../constants/types'
import { getDappNetworkIds } from '../../../../utils/dapp-utils'

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

  const [visibleNetworks, visibleNetworkIds] = React.useMemo(() => {
    const visibleNetworks = networks.filter(
      (network) =>
        !filteredOutNetworkKeys.includes(
          networkEntityAdapter.selectId(network).toString()
        )
    )
    return [visibleNetworks, visibleNetworks.map(networkEntityAdapter.selectId)]
  }, [networks, filteredOutNetworkKeys])

  const visibleDapps = React.useMemo(() => {
    if (!topDapps) {
      return []
    }

    return topDapps.filter((dapp: BraveWallet.Dapp) => {
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
  }, [filteredOutCategories, topDapps, visibleNetworkIds, visibleNetworks])

  const searchedDapps = React.useMemo(() => {
    if (!visibleDapps) {
      return []
    }

    const searchValueLower = searchValue.toLowerCase().trim()

    if (!searchValueLower) {
      return visibleDapps
    }

    return visibleDapps.filter((dapp) => {
      return (
        dapp.name.toLowerCase().includes(searchValueLower) ||
        dapp.description.toLowerCase().includes(searchValueLower)
      )
    })
  }, [visibleDapps, searchValue])

  const dappCategories = React.useMemo(() => {
    if (!topDapps) {
      return []
    }

    const categories = new Set<string>()
    topDapps.forEach((dapp) => {
      dapp.categories.forEach((category) => {
        categories.add(category)
      })
    })

    return Array.from(categories)
  }, [topDapps])

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

        <DividerLine />

        {searchedDapps.length ? (
          <VirtualizedDappsList
            dappsList={searchedDapps}
            onClickDapp={(dappId) => {
              history.push(makeDappDetailsRoute(dappId.toString()))
            }}
          />
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
