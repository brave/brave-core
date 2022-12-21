// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector, useDispatch } from 'react-redux'

// Types
import { BraveWallet, SupportedTestNetworks, WalletAccountType, WalletState } from '../../../constants/types'

// Components
import NetworkFilterItem from './network-filter-item'
import { CreateNetworkIcon } from '../../shared'

// Utils
import { WalletActions } from '../../../common/actions'
import { getLocale } from '../../../../common/locale'
import { accountInfoEntityAdaptor } from '../../../common/slices/entities/account-info.entity'

// Options
import {
  AllNetworksOption,
  SupportedTopLevelChainIds
} from '../../../options/network-filter-options'
import { AllAccountsOption } from '../../../options/account-filter-options'

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
  selectedAccount?: Pick<WalletAccountType, 'address' | 'coin' | 'name'>
  onSelectNetwork?: (network: BraveWallet.NetworkInfo) => void
}

export const NetworkFilterSelector = ({
  networkListSubset,
  onSelectNetwork,
  selectedNetwork: networkProp,
  selectedAccount: accountProp
}: Props) => {
  // state
  const [showNetworkFilter, setShowNetworkFilter] = React.useState<boolean>(false)

  // redux
  const dispatch = useDispatch()
  const selectedNetworkFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter)
  const selectedAccountFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedAccountFilter)
  const reduxNetworkList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)

  // api
  const selectedNetwork = networkProp || selectedNetworkFilter
  const selectedAccount = accountProp || selectedAccountFilter

  // memos
  const networkList: BraveWallet.NetworkInfo[] = React.useMemo(() => {
    // Filters networks by coinType if a selectedAccountFilter is selected
    const accountId = accountInfoEntityAdaptor.selectId(selectedAccount)
    const networks = accountId === AllAccountsOption.id
      ? networkListSubset
      : networkListSubset?.filter((network) => network.coin === selectedAccount.coin)
    return networks || reduxNetworkList
  }, [networkListSubset, reduxNetworkList, selectedAccount])

  const sortedNetworks = React.useMemo(() => {
    const onlyMainnets = networkList.filter((network) => SupportedTopLevelChainIds.includes(network.chainId))
    const removedMainnets = networkList.filter((network) => !SupportedTopLevelChainIds.includes(network.chainId))
    return [AllNetworksOption, ...onlyMainnets, ...removedMainnets]
  }, [networkList])

  const primaryNetworks = React.useMemo(() => {
    const onlyMainnets = networkList.filter((network) => SupportedTopLevelChainIds.includes(network.chainId))
    return [AllNetworksOption, ...onlyMainnets]
  }, [sortedNetworks])

  const secondaryNetworks = React.useMemo(() => {
    const primaryList = [AllNetworksOption.chainId, ...SupportedTopLevelChainIds, ...SupportedTestNetworks]
    return sortedNetworks.filter((network) => !primaryList.includes(network.chainId))
  }, [sortedNetworks])

  const testNetworks = React.useMemo(() => {
    return networkList.filter((network) => SupportedTestNetworks.includes(network.chainId))
  }, [networkList])

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
      dispatch(WalletActions.setSelectedNetworkFilter(network))
    }

    hideNetworkFilter()
  }, [onSelectNetwork, hideNetworkFilter])

  // render
  return (
    <StyledWrapper>
      <DropDownButton
        onClick={toggleShowNetworkFilter}>
        <SelectorLeftSide>
          {selectedNetwork.chainId !== AllNetworksOption.chainId &&
            <CreateNetworkIcon network={selectedNetwork} marginRight={14} size='big' />
          }
          {selectedNetwork.chainName}
        </SelectorLeftSide>
        <DropDownIcon />
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
                  selectedNetwork={selectedNetworkFilter}
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
