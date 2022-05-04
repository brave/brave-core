import * as React from 'react'
import { useSelector, useDispatch } from 'react-redux'

// Types
import { BraveWallet, SupportedTestNetworks, WalletState } from '../../../constants/types'

// Components
import NetworkFilterItem from './network-filter-item'
import { CreateNetworkIcon } from '../../shared'

// Utils
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
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
  LeftSide,
  SubDropDown,
  SecondaryNetworkText,
  ClickAwayArea
} from './style'

function NetworkFilterSelector () {
  const [showNetworkFilter, setShowNetworkFilter] = React.useState<boolean>(false)

  // redux
  const {
    selectedNetworkFilter,
    networkList,
    isTestNetworksEnabled
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const dispatch = useDispatch()

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

  const toggleShowNetworkFilter = () => {
    setShowNetworkFilter(!showNetworkFilter)
  }

  const onSelectAndClose = (network: BraveWallet.NetworkInfo) => {
    dispatch(WalletActions.setSelectedNetworkFilter(network))
    toggleShowNetworkFilter()
  }

  const hideNetworkFilter = () => {
    setShowNetworkFilter(false)
  }

  return (
    <StyledWrapper>
      <DropDownButton onClick={toggleShowNetworkFilter}>
        <LeftSide>
          {selectedNetworkFilter.chainId !== AllNetworksOption.chainId &&
            <CreateNetworkIcon network={selectedNetworkFilter} marginRight={14} size='big' />
          }
          {selectedNetworkFilter.chainId !== AllNetworksOption.chainId ? reduceNetworkDisplayName(selectedNetworkFilter.chainName) : selectedNetworkFilter.chainName}
        </LeftSide>
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
                    n.symbol.toLowerCase() === network.symbol.toLowerCase())
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
