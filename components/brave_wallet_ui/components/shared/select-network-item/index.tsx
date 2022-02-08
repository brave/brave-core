import * as React from 'react'
import { BraveWallet } from '../../../constants/types'
import { CreateNetworkIcon } from '../'
// Styled Components
import {
  StyledWrapper,
  NetworkName,
  LeftSide,
  BigCheckMark
} from './style'

export interface Props {
  selectedNetwork: BraveWallet.EthereumChain
  network: BraveWallet.EthereumChain
  onSelectNetwork: () => void
}

function SelectNetworkItem (props: Props) {
  const { network, onSelectNetwork, selectedNetwork } = props

  return (
    <StyledWrapper onClick={onSelectNetwork}>
      <LeftSide>
        <CreateNetworkIcon network={network} marginRight={14} />
        <NetworkName>{network.chainName}</NetworkName>
      </LeftSide>
      {selectedNetwork.chainId === network.chainId &&
        <BigCheckMark />
      }
    </StyledWrapper>
  )
}

export default SelectNetworkItem
