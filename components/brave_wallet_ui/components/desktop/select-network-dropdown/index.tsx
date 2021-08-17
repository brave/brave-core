import * as React from 'react'
import { EthereumChain } from '../../../constants/types'
import { SelectNetwork } from '../../shared'
// Styled Components
import {
  StyledWrapper,
  OvalButton,
  OvalButtonText,
  CaratDownIcon,
  DropDown
} from './style'

export interface Props {
  onSelectNetwork: (network: EthereumChain) => () => void
  networkList: EthereumChain[]
  selectedNetwork: EthereumChain
  showNetworkDropDown: boolean
  onClick: () => void
}

function SelectNetworkDropdown (props: Props) {
  const { selectedNetwork, networkList, onClick, onSelectNetwork, showNetworkDropDown } = props

  return (
    <StyledWrapper>
      <OvalButton onClick={onClick}>
        <OvalButtonText>{selectedNetwork.chainName}</OvalButtonText>
        <CaratDownIcon />
      </OvalButton>
      {showNetworkDropDown &&
        <DropDown>
          <SelectNetwork
            networks={networkList}
            onSelectNetwork={onSelectNetwork}
          />
        </DropDown>
      }
    </StyledWrapper >
  )
}

export default SelectNetworkDropdown
