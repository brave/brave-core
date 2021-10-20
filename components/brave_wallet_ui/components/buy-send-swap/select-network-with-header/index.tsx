import * as React from 'react'
import { EthereumChain } from '../../../constants/types'
import { SelectNetwork } from '../../shared'
import Header from '../select-header'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollContainer
} from '../shared-styles'

export interface Props {
  networks: EthereumChain[]
  onSelectNetwork: (network: EthereumChain) => () => void
  onBack: () => void
}

function SelectNetworkWithHeader (props: Props) {
  const { networks, onSelectNetwork, onBack } = props
  return (
    <SelectWrapper>
      <Header title={getLocale('braveWalletSelectNetwork')} onBack={onBack} />
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
