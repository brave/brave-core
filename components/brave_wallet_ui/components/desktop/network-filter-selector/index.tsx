// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet, SupportedTestNetworks } from '../../../constants/types'

// Components
import NetworkFilterItem from './network-filter-item'
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'

// Hooks
import { useGetVisibleNetworksQuery } from '../../../common/slices/api.slice'

// Utils
import { getLocale } from '../../../../common/locale'

// Options
import {
  AllNetworksOption,
  SupportedTopLevelChainIds
} from '../../../options/network-filter-options'
import {
  AllAccountsOptionUniqueKey //
} from '../../../options/account-filter-options'

// Styled Components
import {
  StyledWrapper,
  DropDown,
  DropDownButton,
  DropDownIcon,
  SelectorLeftSide,
  SecondaryNetworkText,
  ClickAwayArea
} from './style'

interface Props {
  networkListSubset?: BraveWallet.NetworkInfo[]
  selectedNetwork?: BraveWallet.NetworkInfo
  selectedAccount?: Pick<
    BraveWallet.AccountInfo,
    'accountId' | 'address' | 'name'
  >
  isV2?: boolean
  disableAllAccountsOption?: boolean
  onSelectNetwork: (network: BraveWallet.NetworkInfo) => void
  dropdownPosition?: 'left' | 'right'
}

export const NetworkFilterSelector = ({
  networkListSubset,
  onSelectNetwork,
  selectedNetwork = AllNetworksOption,
  isV2,
  selectedAccount,
  disableAllAccountsOption,
  dropdownPosition
}: Props) => {
  // state
  const [showNetworkFilter, setShowNetworkFilter] =
    React.useState<boolean>(false)

  // queries
  const { data: reduxNetworkList } = useGetVisibleNetworksQuery()

  const networks = networkListSubset?.length
    ? networkListSubset
    : reduxNetworkList

  // memos
  const filteredNetworks: BraveWallet.NetworkInfo[] = React.useMemo(() => {
    // Filters networks by coinType if a selectedAccountFilter is selected
    return selectedAccount &&
      selectedAccount.accountId.uniqueKey !== AllAccountsOptionUniqueKey
      ? networks?.filter(
          (network) => network.coin === selectedAccount.accountId.coin
        )
      : networks
  }, [networks, selectedAccount])

  const { primaryNetworks, secondaryNetworks, testNetworks } =
    React.useMemo(() => {
      const primaryNetworks: BraveWallet.NetworkInfo[] =
        disableAllAccountsOption ? [] : [AllNetworksOption]
      const secondaryNetworks: BraveWallet.NetworkInfo[] = []
      const testNetworks: BraveWallet.NetworkInfo[] = []

      for (const network of filteredNetworks) {
        switch (true) {
          case SupportedTopLevelChainIds.includes(network.chainId): {
            primaryNetworks.push(network)
            break
          }

          case SupportedTestNetworks.includes(network.chainId): {
            testNetworks.push(network)
            break
          }

          case network.chainId === AllNetworksOption.chainId:
            break // pre-sorted

          default: {
            secondaryNetworks.push(network)
            break
          }
        }
      }

      return {
        primaryNetworks,
        secondaryNetworks,
        testNetworks
      }
    }, [filteredNetworks, disableAllAccountsOption])

  const toggleShowNetworkFilter = React.useCallback(() => {
    setShowNetworkFilter((prev) => !prev)
  }, [])

  const hideNetworkFilter = React.useCallback(() => {
    setShowNetworkFilter(false)
  }, [])

  const onSelectAndClose = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      onSelectNetwork(network)
      hideNetworkFilter()
    },
    [onSelectNetwork, hideNetworkFilter]
  )

  // render
  return (
    <StyledWrapper>
      <DropDownButton
        isV2={isV2}
        onClick={toggleShowNetworkFilter}
      >
        <SelectorLeftSide>
          {selectedNetwork.chainId !== AllNetworksOption.chainId && (
            <CreateNetworkIcon
              network={selectedNetwork}
              marginRight={14}
              size='small'
            />
          )}
          {selectedNetwork.chainName}
        </SelectorLeftSide>
        <DropDownIcon isV2={isV2} />
      </DropDownButton>

      {showNetworkFilter && (
        <DropDown dropdownPosition={dropdownPosition}>
          {primaryNetworks.map((network: BraveWallet.NetworkInfo) => (
            <NetworkFilterItem
              key={`${network.chainId + network.chainName}`}
              network={network}
              onSelectNetwork={onSelectAndClose}
              isSelected={
                network.chainId === selectedNetwork.chainId &&
                network.symbol.toLowerCase() ===
                  selectedNetwork.symbol.toLowerCase()
              }
            ></NetworkFilterItem>
          ))}

          {secondaryNetworks.length > 0 && (
            <>
              <SecondaryNetworkText>
                {getLocale('braveWalletNetworkFilterSecondary')}
              </SecondaryNetworkText>
              {secondaryNetworks.map((network) => (
                <NetworkFilterItem
                  key={`${network.chainId + network.chainName}`}
                  network={network}
                  onSelectNetwork={onSelectAndClose}
                  isSelected={
                    network.chainId === selectedNetwork.chainId &&
                    network.symbol.toLowerCase() ===
                      selectedNetwork.symbol.toLowerCase()
                  }
                />
              ))}
            </>
          )}

          {testNetworks.length > 0 && (
            <>
              <SecondaryNetworkText>
                {getLocale('braveWalletNetworkFilterTestNetworks')}
              </SecondaryNetworkText>
              {testNetworks.map((network) => (
                <NetworkFilterItem
                  key={`${network.chainId + network.chainName}`}
                  network={network}
                  onSelectNetwork={onSelectAndClose}
                  isSelected={
                    network.chainId === selectedNetwork.chainId &&
                    network.symbol.toLowerCase() ===
                      selectedNetwork.symbol.toLowerCase()
                  }
                />
              ))}
            </>
          )}
        </DropDown>
      )}
      {showNetworkFilter && <ClickAwayArea onClick={hideNetworkFilter} />}
    </StyledWrapper>
  )
}

export default NetworkFilterSelector
