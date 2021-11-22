import * as React from 'react'
import { EthereumChain } from '../../../constants/types'
import { SelectNetwork, Tooltip } from '../../shared'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
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
      <Tooltip
        text={selectedNetwork.chainName}
      >
        <OvalButton onClick={onClick}>
          <OvalButtonText>{reduceNetworkDisplayName(selectedNetwork.chainName)}</OvalButtonText>
          <CaratDownIcon />
        </OvalButton>
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
