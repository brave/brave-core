// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector, useDispatch } from 'react-redux'

// Types
import {
  BraveWallet,
  SupportedTestNetworks,
  WalletState
} from '../../../constants/types'
import { LOCAL_STORAGE_KEYS } from '../../../common/constants/local-storage-keys'

// Components
import NetworkFilterItem from './network-filter-item'
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'

// Utils
import { WalletActions } from '../../../common/actions'
import { getLocale } from '../../../../common/locale'

// Options
import {
  AllNetworksOption,
  SupportedTopLevelChainIds
} from '../../../options/network-filter-options'
import { applySelectedAccountFilter } from '../../../options/account-filter-options'

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
import {
  useGetNetworkQuery,
  useGetVisibleNetworksQuery
} from '../../../common/slices/api.slice'
import { skipToken } from '@reduxjs/toolkit/dist/query'

interface Props {
  networkListSubset?: BraveWallet.NetworkInfo[]
  selectedNetwork?: BraveWallet.NetworkInfo
  selectedAccount?: Pick<BraveWallet.AccountInfo,
    | 'accountId'
    | 'address'
    | 'name'
  >
  isV2?: boolean
  onSelectNetwork?: (network: BraveWallet.NetworkInfo) => void
}

export const NetworkFilterSelector = ({
  networkListSubset,
  onSelectNetwork,
  selectedNetwork: networkProp,
  selectedAccount: accountProp,
  isV2
}: Props) => {
  // state
  const [showNetworkFilter, setShowNetworkFilter] = React.useState<boolean>(false)

  // redux
  const dispatch = useDispatch()
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)
  const selectedNetworkFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter)
  const selectedAccountFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedAccountFilter)

  // queries
  const { data: reduxNetworkList } = useGetVisibleNetworksQuery(undefined, {
    skip: !!networkListSubset
  })
  const { data: selectedNetworkFromFilter } = useGetNetworkQuery(
    !!networkProp ||
      !selectedNetworkFilter ||
      selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? skipToken
      : selectedNetworkFilter
  )

  const selectedNetwork =
    networkProp ||
    (selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? AllNetworksOption
      : selectedNetworkFromFilter || AllNetworksOption)

  const oneFilteredAccount = React.useMemo(() => {
    return (
      accountProp ||
      applySelectedAccountFilter(accounts, selectedAccountFilter).oneAccount
    )
  }, [accountProp, accounts, selectedAccountFilter])

  // memos
  const filteredNetworks: BraveWallet.NetworkInfo[] = React.useMemo(() => {
    // Filters networks by coinType if a selectedAccountFilter is selected
    const networks = !oneFilteredAccount
      ? networkListSubset
      : networkListSubset?.filter(
          (network) => network.coin === oneFilteredAccount.accountId.coin
        )
    return networks || reduxNetworkList
  }, [networkListSubset, reduxNetworkList, oneFilteredAccount])

  const sortedNetworks = React.useMemo(() => {
    const onlyMainnets = filteredNetworks.filter((network) =>
      SupportedTopLevelChainIds.includes(network.chainId)
    )
    const removedMainnets = filteredNetworks.filter(
      (network) => !SupportedTopLevelChainIds.includes(network.chainId)
    )
    return [AllNetworksOption, ...onlyMainnets, ...removedMainnets]
  }, [filteredNetworks])

  const primaryNetworks = React.useMemo(() => {
    const onlyMainnets = filteredNetworks.filter((network) =>
      SupportedTopLevelChainIds.includes(network.chainId)
    )
    return [AllNetworksOption, ...onlyMainnets]
  }, [sortedNetworks])

  const secondaryNetworks = React.useMemo(() => {
    const primaryList = [AllNetworksOption.chainId, ...SupportedTopLevelChainIds, ...SupportedTestNetworks]
    return sortedNetworks.filter((network) => !primaryList.includes(network.chainId))
  }, [sortedNetworks])

  const testNetworks = React.useMemo(() => {
    return filteredNetworks.filter((network) =>
      SupportedTestNetworks.includes(network.chainId)
    )
  }, [filteredNetworks])

  const toggleShowNetworkFilter = React.useCallback(() => {
    setShowNetworkFilter(prev => !prev)
  }, [])

  const hideNetworkFilter = React.useCallback(() => {
    setShowNetworkFilter(false)
  }, [])

  const onSelectAndClose = React.useCallback((network: BraveWallet.NetworkInfo) => {
    if (onSelectNetwork) {
      onSelectNetwork(network)
    } else {
      const networkFilter = {
        chainId: network.chainId,
        coin: network.coin
      }
      window.localStorage.setItem(LOCAL_STORAGE_KEYS.PORTFOLIO_NETWORK_FILTER_OPTION, JSON.stringify(networkFilter))
      dispatch(WalletActions.setSelectedNetworkFilter(networkFilter))
    }

    hideNetworkFilter()
  }, [onSelectNetwork, hideNetworkFilter])

  // render
  return (
    <StyledWrapper>
      <DropDownButton
        isV2={isV2}
        onClick={toggleShowNetworkFilter}
      >
        <SelectorLeftSide>
          {selectedNetwork.chainId !== AllNetworksOption.chainId &&
            <CreateNetworkIcon network={selectedNetwork} marginRight={14} size='small' />
          }
          {selectedNetwork.chainName}
        </SelectorLeftSide>
        <DropDownIcon isV2={isV2} />
      </DropDownButton>

      {showNetworkFilter &&
        <DropDown>
          {primaryNetworks.map((network: BraveWallet.NetworkInfo) =>
            <NetworkFilterItem
              key={`${network.chainId + network.chainName}`}
              network={network}
              onSelectNetwork={onSelectAndClose}
              selectedNetwork={selectedNetwork}
            >
            </NetworkFilterItem>
          )}

          {secondaryNetworks.length > 0 &&
            <>
              <SecondaryNetworkText>{getLocale('braveWalletNetworkFilterSecondary')}</SecondaryNetworkText>
              {secondaryNetworks.map((network) =>
                <NetworkFilterItem
                  key={`${network.chainId + network.chainName}`}
                  network={network}
                  onSelectNetwork={onSelectAndClose}
                  selectedNetwork={selectedNetwork}
                />
              )}
            </>
          }

          {testNetworks.length > 0 &&
            <>
              <SecondaryNetworkText>{getLocale('braveWalletNetworkFilterTestNetworks')}</SecondaryNetworkText>
              {testNetworks.map((network) =>
                <NetworkFilterItem
                  key={`${network.chainId + network.chainName}`}
                  network={network}
                  onSelectNetwork={onSelectAndClose}
                  selectedNetwork={selectedNetwork}
                />
              )}
            </>
          }
        </DropDown>
      }
      {showNetworkFilter &&
        <ClickAwayArea onClick={hideNetworkFilter} />
      }
    </StyledWrapper >
  )
}

export default NetworkFilterSelector
