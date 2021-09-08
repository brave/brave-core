import * as React from 'react'
import { EthereumChain } from '../../../constants/types'
import { create } from 'ethereum-blockies'
// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  NetworkName
} from './style'

export interface Props {
  network: EthereumChain
  onSelectNetwork: () => void
}

function SelectNetworkItem (props: Props) {
  const { network, onSelectNetwork } = props

  const orb = React.useMemo(() => {
    return create({ seed: network.chainName, size: 8, scale: 16 }).toDataURL()
  }, [network])

  return (
    <StyledWrapper onClick={onSelectNetwork}>
      <AccountCircle orb={orb} />
      <NetworkName>{network.chainName}</NetworkName>
    </StyledWrapper>
  )
}

export default SelectNetworkItem
