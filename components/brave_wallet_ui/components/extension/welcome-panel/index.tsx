import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  PageIcon,
  RestoreButton
} from './style'
import { NavButton } from '../'
import { getLocale } from '../../../../common/locale'

export interface Props {
  onSetup: () => void
  onRestore: () => void
}

function WelcomePanel (props: Props) {
  const { onRestore, onSetup } = props
  return (
    <StyledWrapper>
      <PageIcon />
      <Title>{getLocale('braveWalletWelcomeTitle')}</Title>
      <Description>{getLocale('braveWalletWelcomeDescription')}</Description>
      <NavButton buttonType='primary' text={getLocale('braveWalletWelcomeButton')} onSubmit={onSetup} />
      <RestoreButton onClick={onRestore}>{getLocale('braveWalletWelcomeRestoreButton')}</RestoreButton>
    </StyledWrapper>
  )
}

export default WelcomePanel
