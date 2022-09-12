import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Components
import { CreateNetworkIcon } from '../../shared'

// Utils
import { reduceNetworkDisplayName } from '../../../utils/network-utils'

// Options
import { AllNetworksOption } from '../../../options/network-filter-options'

// Styled Components
import {
  NetworkItemButton,
  NetworkName,
  LeftSide,
  NetworkItemWrapper,
  BigCheckMark
} from './style'

export interface Props {
  children?: React.ReactNode
  selectedNetwork: BraveWallet.NetworkInfo
  network: BraveWallet.NetworkInfo
  isSubItem: boolean
  onSelectNetwork: (network?: BraveWallet.NetworkInfo) => void
}

function NetworkFilterItem (props: Props) {
  const { network, onSelectNetwork, children, selectedNetwork, isSubItem } = props
  const [showSubMenu, setShowSubMenu] = React.useState<boolean>(false)

  const showTip = () => {
    if (!isSubItem) {
      setShowSubMenu(true)
    }
  }

  const hideTip = () => {
    if (!isSubItem) {
      setShowSubMenu(false)
    }
  }

  const onClickSelectNetwork = () => {
    setShowSubMenu(false)
    onSelectNetwork(network)
  }

  return (
    <NetworkItemWrapper
      onMouseEnter={showTip}
      onMouseLeave={hideTip}
    >
      <NetworkItemButton onClick={onClickSelectNetwork}>
        <LeftSide>
          {network.chainId !== AllNetworksOption.chainId &&
            <CreateNetworkIcon network={network} marginRight={14} size='big' />
          }
          <NetworkName>
            {isSubItem
              ? network.chainName
              : reduceNetworkDisplayName(network.chainName)
            }
          </NetworkName>
        </LeftSide>
        {network.chainId === selectedNetwork.chainId &&
          network.symbol.toLowerCase() === selectedNetwork.symbol.toLowerCase() &&
          isSubItem &&
          <BigCheckMark />
        }
      </NetworkItemButton>
      {showSubMenu && !isSubItem &&
        <>{children}</>
      }
    </NetworkItemWrapper>
  )
}

export default NetworkFilterItem
