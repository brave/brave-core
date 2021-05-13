import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  PageIcon,
  RestoreButton
} from './style'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'

export interface Props {
  onSetup: () => void
  onRestore: () => void
}

function OnboardingWelcome (props: Props) {
  const { onRestore, onSetup } = props
  return (
    <StyledWrapper>
      <PageIcon />
      <Title>{locale.welcomeTitle}</Title>
      <Description>{locale.welcomeDescription}</Description>
      <NavButton buttonType='primary' text={locale.welcomeButton} onSubmit={onSetup} />
      <RestoreButton onClick={onRestore}>{locale.welcomeRestoreButton}</RestoreButton>
    </StyledWrapper>
  )
}

export default OnboardingWelcome
