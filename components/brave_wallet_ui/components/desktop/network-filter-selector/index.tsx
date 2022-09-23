// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector, useDispatch } from 'react-redux'

// Types
import { BraveWallet, SupportedTestNetworks, WalletState } from '../../../constants/types'

// Components
import NetworkFilterItem from './network-filter-item'
import { CreateNetworkIcon } from '../../shared'

// Utils
import { WalletActions } from '../../../common/actions'
import { getLocale } from '../../../../common/locale'
import {
  AllNetworksOption,
  SupportedTopLevelChainIds
} from '../../../options/network-filter-options'

// Styled Components
import {
  StyledWrapper,
  DropDown,
  DropDownButton,
  DropDownIcon,
  SelectorLeftSide,
  SubDropDown,
  SecondaryNetworkText,
  ClickAwayArea
} from './style'

interface Props {
  networkListSubset?: BraveWallet.NetworkInfo[]
}

export const NetworkFilterSelector = ({ networkListSubset }: Props) => {
  // state
  const [showNetworkFilter, setShowNetworkFilter] = React.useState<boolean>(false)

  // redux
  const dispatch = useDispatch()
  const selectedNetworkFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter)
  const reduxNetworkList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)
  const isTestNetworksEnabled = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isTestNetworksEnabled)

  // memos
  const networkList = React.useMemo(() => {
    return networkListSubset || reduxNetworkList
  }, [networkListSubset, reduxNetworkList])

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

  // methods
  const toggleShowNetworkFilter = React.useCallback(() => {
    setShowNetworkFilter(prev => !prev)
  }, [])

  const onSelectAndClose = React.useCallback((network: BraveWallet.NetworkInfo) => {
    dispatch(WalletActions.setSelectedNetworkFilter(network))
    toggleShowNetworkFilter()
  }, [toggleShowNetworkFilter])

  const hideNetworkFilter = React.useCallback(() => {
    setShowNetworkFilter(false)
  }, [])

  // render
  return (
    <StyledWrapper>
      <DropDownButton
        onClick={toggleShowNetworkFilter}>
        <SelectorLeftSide>
          {selectedNetworkFilter.chainId !== AllNetworksOption.chainId &&
            <CreateNetworkIcon network={selectedNetworkFilter} marginRight={14} size='big' />
          }
          {selectedNetworkFilter.chainName}
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
              selectedNetwork={selectedNetworkFilter}
              isSubItem={isTestNetworksEnabled ? !SupportedTopLevelChainIds.includes(network.chainId) : true}
            >
              {isTestNetworksEnabled &&
                <SubDropDown>
                  {sortedNetworks.filter((n) =>
                    n.coin === network.coin &&
                    n.symbol.toLowerCase() === network.symbol.toLowerCase() &&
                    // Optimism's native asset is considered ETH, so we want to make sure
                    // we dont include it under Ethereum's submenu.
                    n.chainId !== BraveWallet.OPTIMISM_MAINNET_CHAIN_ID)
                    .map((subNetwork) =>
                      <NetworkFilterItem
                        key={`${subNetwork.chainId + subNetwork.chainName}`}
                        network={subNetwork}
                        onSelectNetwork={onSelectAndClose}
                        selectedNetwork={selectedNetworkFilter}
                        isSubItem={true}
                      />
                    )}
                </SubDropDown>
              }
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
                  selectedNetwork={selectedNetworkFilter}
                  isSubItem={true}
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
