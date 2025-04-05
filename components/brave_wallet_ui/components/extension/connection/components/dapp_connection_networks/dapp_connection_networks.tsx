// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Queries
import {
  useGetVisibleNetworksQuery //
} from '../../../../../common/slices/api.slice'

// Types
import {
  BraveWallet,
  DAppSupportedCoinTypes,
  DAppSupportedPrimaryChains,
  SupportedTestNetworks
} from '../../../../../constants/types'

// Components
import {
  ChangeNetworkButton //
} from './change_network_button/change_network_button'

// Styled Components
import { Text, Row, Column, ScrollableColumn } from '../../../../shared/style'

interface Props {
  coin: BraveWallet.CoinType
  selectedNetwork?: BraveWallet.NetworkInfo
  onChangeNetwork: (network: BraveWallet.NetworkInfo) => void
}

export const DAppConnectionNetworks = (props: Props) => {
  const { coin, selectedNetwork, onChangeNetwork } = props

  // Queries
  const { data: networkList = [] } = useGetVisibleNetworksQuery()

  // Memos
  const dappSupportedNetwork = React.useMemo(() => {
    return networkList.filter(
      (network) =>
        DAppSupportedCoinTypes.includes(network.coin) && network.coin === coin
    )
  }, [networkList, coin])

  const primaryNetworks = React.useMemo(() => {
    return dappSupportedNetwork.filter((network) =>
      DAppSupportedPrimaryChains.includes(network.chainId)
    )
  }, [dappSupportedNetwork])

  const secondaryNetworks = React.useMemo(() => {
    return dappSupportedNetwork.filter(
      (network) =>
        !DAppSupportedPrimaryChains.includes(network.chainId) &&
        !SupportedTestNetworks.includes(network.chainId)
    )
  }, [dappSupportedNetwork])

  const testNetworks = React.useMemo(() => {
    return dappSupportedNetwork.filter((network) =>
      SupportedTestNetworks.includes(network.chainId)
    )
  }, [dappSupportedNetwork])

  return (
    <ScrollableColumn
      padding='8px'
      gap='32px'
      maxHeight='450px'
      margin='25px 0px 0px 0px'
    >
      {primaryNetworks.length !== 0 && (
        <Column
          width='100%'
          justifyContent='flex-start'
        >
          <Row
            justifyContent='flex-start'
            padding='0px 12px'
            marginBottom='8px'
          >
            <Text
              textSize='12px'
              isBold={true}
              textColor='tertiary'
            >
              {getLocale('braveWalletPrimaryNetworks')}
            </Text>
          </Row>
          {primaryNetworks.map((network: BraveWallet.NetworkInfo) => (
            <ChangeNetworkButton
              key={network.chainId}
              network={network}
              onClick={() => onChangeNetwork(network)}
              isActiveNetwork={selectedNetwork?.chainId === network.chainId}
            />
          ))}
        </Column>
      )}

      {secondaryNetworks.length !== 0 && (
        <Column
          width='100%'
          justifyContent='flex-start'
        >
          <Row
            justifyContent='flex-start'
            padding='0px 12px'
            marginBottom='8px'
          >
            <Text
              textSize='12px'
              isBold={true}
              textColor='tertiary'
            >
              {getLocale('braveWalletNetworkFilterSecondary')}
            </Text>
          </Row>
          {secondaryNetworks.map((network: BraveWallet.NetworkInfo) => (
            <ChangeNetworkButton
              key={network.chainId}
              network={network}
              onClick={() => onChangeNetwork(network)}
              isActiveNetwork={selectedNetwork?.chainId === network.chainId}
            />
          ))}
        </Column>
      )}

      {testNetworks.length !== 0 && (
        <Column
          width='100%'
          justifyContent='flex-start'
        >
          <Row
            justifyContent='flex-start'
            padding='0px 12px'
            marginBottom='8px'
          >
            <Text
              textSize='12px'
              isBold={true}
              textColor='tertiary'
            >
              {getLocale('braveWalletNetworkFilterTestNetworks')}
            </Text>
          </Row>
          {testNetworks.map((network: BraveWallet.NetworkInfo) => (
            <ChangeNetworkButton
              key={network.chainId}
              network={network}
              onClick={() => onChangeNetwork(network)}
              isActiveNetwork={selectedNetwork?.chainId === network.chainId}
            />
          ))}
        </Column>
      )}
    </ScrollableColumn>
  )
}
