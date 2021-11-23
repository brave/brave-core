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
  selectedNetwork: EthereumChain
  onSelectNetwork: (network: EthereumChain) => () => void
  onBack: () => void
}

function SelectNetworkWithHeader (props: Props) {
  const { networks, onSelectNetwork, onBack, selectedNetwork } = props
  return (
    <SelectWrapper>
      <Header title={getLocale('braveWalletSelectNetwork')} onBack={onBack} />
      <SelectScrollContainer>
        <SelectNetwork
          selectedNetwork={selectedNetwork}
          networks={networks}
          onSelectNetwork={onSelectNetwork}
        />
      </SelectScrollContainer>
    </SelectWrapper>
  )
}

export default SelectNetworkWithHeader
