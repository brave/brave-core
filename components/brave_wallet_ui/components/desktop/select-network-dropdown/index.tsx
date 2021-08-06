import * as React from 'react'
import { Network } from '../../../constants/types'
import { NetworkOptions } from '../../../options/network-options'
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
  onSelectNetwork: (network: Network) => () => void
  selectedNetwork: Network
  showNetworkDropDown: boolean
  onClick: () => void
}

function SelectNetworkDropdown (props: Props) {
  const { selectedNetwork, onClick, onSelectNetwork, showNetworkDropDown } = props

  return (
    <StyledWrapper>
      <OvalButton onClick={onClick}>
        <OvalButtonText>{NetworkOptions[selectedNetwork].abbr}</OvalButtonText>
        <CaratDownIcon />
      </OvalButton>
      {showNetworkDropDown &&
        <DropDown>
          <SelectNetwork
            networks={NetworkOptions}
            onSelectNetwork={onSelectNetwork}
          />
        </DropDown>
      }
    </StyledWrapper >
  )
}

export default SelectNetworkDropdown
