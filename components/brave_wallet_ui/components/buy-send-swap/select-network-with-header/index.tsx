import * as React from 'react'
import { Network, NetworkOptionsType } from '../../../constants/types'
import { SelectNetwork } from '../../shared'
import Header from '../select-header'
import locale from '../../../constants/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollContainer
} from '../shared-styles'

export interface Props {
  networks: NetworkOptionsType[]
  onSelectNetwork: (network: Network) => () => void
  onBack: () => void
}

function SelectNetworkWithHeader (props: Props) {
  const { networks, onSelectNetwork, onBack } = props
  return (
    <SelectWrapper>
      <Header title={locale.selectNetwork} onBack={onBack} />
      <SelectScrollContainer>
        <SelectNetwork
          networks={networks}
          onSelectNetwork={onSelectNetwork}
        />
      </SelectScrollContainer>
    </SelectWrapper>
  )
}

export default SelectNetworkWithHeader
