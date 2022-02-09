import * as React from 'react'
import { BraveWallet } from '../../../constants/types'
import { SelectNetwork, Tooltip, SelectNetworkButton } from '../../shared'
// Styled Components
import {
  StyledWrapper,
  DropDown
} from './style'

export interface Props {
  onSelectNetwork: (network: BraveWallet.EthereumChain) => () => void
  networkList: BraveWallet.EthereumChain[]
  selectedNetwork: BraveWallet.EthereumChain
  showNetworkDropDown: boolean
  onClick: () => void
}

function SelectNetworkDropdown (props: Props) {
  const { selectedNetwork, networkList, onClick, onSelectNetwork, showNetworkDropDown } = props

  return (
    <StyledWrapper>
      <Tooltip
        text={selectedNetwork.chainName}
      >
        <SelectNetworkButton
          onClick={onClick}
          selectedNetwork={selectedNetwork}
        />
      </Tooltip>
      {showNetworkDropDown &&
        <DropDown>
          <SelectNetwork
            selectedNetwork={selectedNetwork}
            networks={networkList}
            onSelectNetwork={onSelectNetwork}
          />
        </DropDown>
      }
    </StyledWrapper >
  )
}

export default SelectNetworkDropdown
