import * as React from 'react'
import { SelectNetwork } from '../../shared'
import Header from '../select-header'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollContainer
} from '../shared-styles'

export interface Props {
  hasAddButton?: boolean
  onBack: () => void
  onAddNetwork?: () => void
}

function SelectNetworkWithHeader (props: Props) {
  const { onBack, hasAddButton, onAddNetwork } = props

  return (
    <SelectWrapper>
      <Header
        title={getLocale('braveWalletSelectNetwork')}
        onBack={onBack}
        hasAddButton={hasAddButton}
        onClickAdd={onAddNetwork} />
      <SelectScrollContainer>
        <SelectNetwork />
      </SelectScrollContainer>
    </SelectWrapper>
  )
}

export default SelectNetworkWithHeader
