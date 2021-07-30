import * as React from 'react'
import { NetworkOptionsType } from '../../../constants/types'
import { create } from 'ethereum-blockies'
// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  NetworkName
} from './style'

export interface Props {
  network: NetworkOptionsType
  onSelectNetwork: () => void
}

function SelectNetworkItem (props: Props) {
  const { network, onSelectNetwork } = props

  const orb = React.useMemo(() => {
    return create({ seed: network.name, size: 8, scale: 16 }).toDataURL()
  }, [network])

  return (
    <StyledWrapper onClick={onSelectNetwork}>
      <AccountCircle orb={orb} />
      <NetworkName>{network.name}</NetworkName>
    </StyledWrapper>
  )
}

export default SelectNetworkItem
