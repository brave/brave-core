// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  useGetExternalRewardsWalletQuery,
  useGetRewardsEnabledQuery,
  useGetVisibleNetworksQuery
} from '../../../../../common/slices/api.slice'

// Types
import {
  SupportedTestNetworks
} from '../../../../../constants/types'
import {
  WalletStatus
} from '../../../../../common/async/brave_rewards_api_proxy'

// Options
import {
  SupportedTopLevelChainIds
} from '../../../../../options/network-filter-options'

// Utils
import {
  networkEntityAdapter
} from '../../../../../common/slices/entities/network.entity'
import {
  getNormalizedExternalRewardsNetwork
} from '../../../../../utils/rewards_utils'
import { getLocale } from '../../../../../../common/locale'

// Components
import {
  NetworkCheckboxes
} from './network-chekboxes'

// Styled Components
import {
  SelectAllButton,
  Title,
} from './filter-components.style'
import {
  Row
} from '../../../../shared/style'

interface Props {
  filteredOutNetworkKeys: string[]
  setFilteredOutNetworkKeys: (keys: string[]) => void
}

export const FilterNetworksSection = (props: Props) => {
  const {
    filteredOutNetworkKeys,
    setFilteredOutNetworkKeys
  } = props

  // Queries
  const { data: networks } = useGetVisibleNetworksQuery()
  const { data: isRewardsEnabled } = useGetRewardsEnabledQuery()
  const { data: externalRewardsInfo } = useGetExternalRewardsWalletQuery()

  // Memos
  const primaryNetworks = React.useMemo(() => {
    return networks.filter((network) =>
      SupportedTopLevelChainIds.includes(network.chainId)
    )
  }, [networks])

  const secondaryNetworks = React.useMemo(() => {
    return networks
      .filter(
        (network) =>
          !SupportedTopLevelChainIds.includes(network.chainId) &&
          !SupportedTestNetworks.includes(network.chainId)
      )
  }, [networks])

  const testNetworks = React.useMemo(() => {
    return networks
      .filter(
        (network) =>
          SupportedTestNetworks.includes(network.chainId)
      )
  }, [networks])

  // Computed
  const providerNetwork =
    isRewardsEnabled &&
      externalRewardsInfo?.status === WalletStatus.kConnected
      ? getNormalizedExternalRewardsNetwork(
        externalRewardsInfo?.provider ?? undefined
      )
      : undefined

  const isSelectAll = React.useMemo(() => {
    return filteredOutNetworkKeys.length > 0 && networks
      .some(
        (network) =>
          filteredOutNetworkKeys
            .includes(
              networkEntityAdapter
                .selectId(network)
                .toString()
            )
      )
  }, [networks, filteredOutNetworkKeys])

  const isNetworkFilteredOut = React.useCallback(
    (key: string) => {
      return filteredOutNetworkKeys.includes(key)
    }, [filteredOutNetworkKeys])

  const onCheckNetwork = React.useCallback((key: string) => {
    if (isNetworkFilteredOut(key)) {
      setFilteredOutNetworkKeys(
        filteredOutNetworkKeys
          .filter((networkKey) => networkKey !== key)
      )
      return
    }
    setFilteredOutNetworkKeys([...filteredOutNetworkKeys, key])
  }, [
    filteredOutNetworkKeys,
    isNetworkFilteredOut,
    setFilteredOutNetworkKeys
  ])

  const onSelectOrDeselectAllNetworks = React.useCallback(
    () => {
      if (isSelectAll) {
        setFilteredOutNetworkKeys([])
        return
      }
      setFilteredOutNetworkKeys(
        networks
          .map((network) =>
            networkEntityAdapter
              .selectId(network)
              .toString()
          ))
    }, [
    networks,
    filteredOutNetworkKeys,
    setFilteredOutNetworkKeys,
    isSelectAll
  ])

  return (
    <>
      <Row
        marginBottom={8}
        justifyContent='space-between'
      >
        <Title
          textSize='16px'
          isBold={true}
        >
          {getLocale('braveWalletSelectNetworks')}
        </Title>
        <SelectAllButton
          onClick={onSelectOrDeselectAllNetworks}
        >
          {
            isSelectAll
              ? getLocale('braveWalletSelectAll')
              : getLocale('braveWalletDeselectAll')
          }
        </SelectAllButton>
      </Row>

      {/* Primary Networks */}
      {primaryNetworks.length > 0 &&
        <NetworkCheckboxes
          isNetworkFilteredOut={isNetworkFilteredOut}
          onCheckNetwork={onCheckNetwork}
          networks={primaryNetworks}
          title={getLocale('braveWalletPrimaryNetworks')}
          marginBottom={8}
        />
      }

      {/* Secondary Networks */}
      {secondaryNetworks.length > 0 &&
        <NetworkCheckboxes
          isNetworkFilteredOut={isNetworkFilteredOut}
          onCheckNetwork={onCheckNetwork}
          networks={secondaryNetworks}
          title={getLocale('braveWalletNetworkFilterSecondary')}
          marginBottom={8}
        />
      }

      {/* Test Networks */}
      {testNetworks.length > 0 &&
        <NetworkCheckboxes
          isNetworkFilteredOut={isNetworkFilteredOut}
          onCheckNetwork={onCheckNetwork}
          networks={testNetworks}
          title={getLocale('braveWalletNetworkFilterTestNetworks')}
          marginBottom={8}
        />
      }

      {/* Provider Networks */}
      {providerNetwork &&
        <NetworkCheckboxes
          isNetworkFilteredOut={isNetworkFilteredOut}
          onCheckNetwork={onCheckNetwork}
          networks={[providerNetwork]}
          title={getLocale('braveWalletPlatforms')}
          marginBottom={0}
        />
      }
    </>
  )
}
