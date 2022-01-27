import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description
} from './style'
import { NavButton } from '../'
import { getLocale } from '../../../../common/locale'

export interface Props {
  onSetup: () => void
}

function WelcomePanel (props: Props) {
  const { onSetup } = props
  return (
    <StyledWrapper>
      <Title>{getLocale('braveWalletPanelTitle')}</Title>
      <Description>{getLocale('braveWalletWelcomePanelDescription')}</Description>
      <NavButton buttonType='primary' text={getLocale('braveWalletWelcomePanelButton')} onSubmit={onSetup} />
    </StyledWrapper>
  )
}

export default WelcomePanel
