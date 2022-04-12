import * as React from 'react'
import { BraveWallet } from '../../../constants/types'
import { SelectNetwork } from '../../shared'
// Styled Components
import {
  StyledWrapper,
  DropDown,
  NetworkButton,
  DropDownIcon
} from './style'

export interface Props {
  selectedNetwork: BraveWallet.NetworkInfo
  showNetworkDropDown: boolean
  onClick: () => void
  onSelectCustomNetwork?: (network: BraveWallet.NetworkInfo) => void
}

function SelectNetworkDropdown (props: Props) {
  const { selectedNetwork, onClick, showNetworkDropDown, onSelectCustomNetwork } = props

  return (
    <StyledWrapper>
      <NetworkButton onClick={onClick}>{selectedNetwork.chainName} <DropDownIcon /></NetworkButton>
      {showNetworkDropDown &&
        <DropDown>
          <SelectNetwork
            onSelectCustomNetwork={onSelectCustomNetwork}
            selectedNetwork={selectedNetwork}
          />
        </DropDown>
      }
    </StyledWrapper >
  )
}

export default SelectNetworkDropdown
