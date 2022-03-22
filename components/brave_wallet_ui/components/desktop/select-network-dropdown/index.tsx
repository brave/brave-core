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
  onSelectNetwork: (network: BraveWallet.NetworkInfo) => () => void
  networkList: BraveWallet.NetworkInfo[]
  selectedNetwork: BraveWallet.NetworkInfo
  showNetworkDropDown: boolean
  onClick: () => void
}

function SelectNetworkDropdown (props: Props) {
  const { selectedNetwork, networkList, onClick, onSelectNetwork, showNetworkDropDown } = props

  return (
    <StyledWrapper>
      <NetworkButton onClick={onClick}>{selectedNetwork.chainName} <DropDownIcon /></NetworkButton>
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
