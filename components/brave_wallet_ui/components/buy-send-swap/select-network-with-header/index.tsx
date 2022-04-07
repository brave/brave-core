import * as React from 'react'
import { useDispatch } from 'react-redux'

import { SelectNetwork } from '../../shared'
import Header from '../select-header'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollContainer
} from '../shared-styles'

import { WalletActions } from '../../../common/actions'
import { PanelActions } from '../../../panel/actions'
import { BraveWallet } from '../../../constants/types'

export interface Props {
  hasAddButton?: boolean
  onBack: () => void
  onAddNetwork?: () => void
}

function SelectNetworkWithHeader ({
  onBack,
  hasAddButton,
  onAddNetwork
}: Props) {
  // redux
  const dispatch = useDispatch()

  // methods
  const onSelectCustomNetwork = React.useCallback((network: BraveWallet.NetworkInfo): void => {
    dispatch(WalletActions.selectNetwork(network))
    dispatch(PanelActions.navigateTo('main'))
    onBack()
  }, [onBack])

  // render
  return (
    <SelectWrapper>
      <Header
        title={getLocale('braveWalletSelectNetwork')}
        onBack={onBack}
        hasAddButton={hasAddButton}
        onClickAdd={onAddNetwork}
      />
      <SelectScrollContainer>
        <SelectNetwork onSelectCustomNetwork={onSelectCustomNetwork} />
      </SelectScrollContainer>
    </SelectWrapper>
  )
}

export default SelectNetworkWithHeader
