import * as React from 'react'
import { EthereumChain } from '../../../constants/types'
import { create } from 'ethereum-blockies'
// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  NetworkName,
  LeftSide,
  BigCheckMark
} from './style'

export interface Props {
  selectedNetwork: EthereumChain
  network: EthereumChain
  onSelectNetwork: () => void
}

function SelectNetworkItem (props: Props) {
  const { network, onSelectNetwork, selectedNetwork } = props

  const orb = React.useMemo(() => {
    return create({ seed: network.chainName, size: 8, scale: 16 }).toDataURL()
  }, [network])

  return (
    <StyledWrapper onClick={onSelectNetwork}>
      <LeftSide>
        <AccountCircle orb={orb} />
        <NetworkName>{network.chainName}</NetworkName>
      </LeftSide>
      {selectedNetwork.chainId === network.chainId &&
        <BigCheckMark />
      }
    </StyledWrapper>
  )
}

export default SelectNetworkItem
