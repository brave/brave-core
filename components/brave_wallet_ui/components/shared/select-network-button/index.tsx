import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceNetworkDisplayName } from '../../../utils/network-utils'

// Components
import { CreateNetworkIcon } from '../'

// Styled Components
import {
  OvalButton,
  OvalButtonText,
  CaratDownIcon
} from './style'

export interface Props {
  onClick: () => void
  selectedNetwork: BraveWallet.EthereumChain
  isPanel?: boolean
}

const SelectNetworkButton = (props: Props) => {
  const {
    onClick,
    selectedNetwork,
    isPanel
  } = props
  return (
    <OvalButton isPanel={isPanel} onClick={onClick}>
      <CreateNetworkIcon network={selectedNetwork} marginRight={4} />
      <OvalButtonText isPanel={isPanel}>{reduceNetworkDisplayName(selectedNetwork.chainName)}</OvalButtonText>
      <CaratDownIcon isPanel={isPanel} />
    </OvalButton>
  )
}
export default SelectNetworkButton
