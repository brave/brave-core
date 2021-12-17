import * as React from 'react'
import { BraveWallet } from '../../../constants/types'
import { SelectNetwork } from '../../shared'
import Header from '../select-header'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollContainer
} from '../shared-styles'

export interface Props {
  networks: BraveWallet.EthereumChain[]
  selectedNetwork: BraveWallet.EthereumChain
  hasAddButton?: boolean
  onSelectNetwork: (network: BraveWallet.EthereumChain) => () => void
  onBack: () => void
  onAddNetwork?: () => void
}

function SelectNetworkWithHeader (props: Props) {
  const { networks, onSelectNetwork, onBack, selectedNetwork, hasAddButton, onAddNetwork } = props
  return (
    <SelectWrapper>
      <Header
        title={getLocale('braveWalletSelectNetwork')}
        onBack={onBack}
        hasAddButton={hasAddButton}
        onClickAdd={onAddNetwork} />
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
