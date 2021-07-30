import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  IconBackground,
  PageIcon,
  TermsRow,
  SkipButton
} from './style'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'
import { Checkbox } from 'brave-ui'

export interface Props {
  onSubmit: () => void
  isBackupTermsAccepted: boolean
  isOnboarding: boolean
  onSubmitTerms: (key: string, selected: boolean) => void
  onCancel: () => void
}

function OnboardingRecovery (props: Props) {
  const { onSubmit, isBackupTermsAccepted, isOnboarding, onSubmitTerms, onCancel } = props

  return (
    <StyledWrapper>
      <IconBackground>
        <PageIcon />
      </IconBackground>
      <Title>{locale.backupIntroTitle}</Title>
      <Description>{locale.backupIntroDescription}</Description>
      <TermsRow>
        <Checkbox value={{ backupTerms: isBackupTermsAccepted }} onChange={onSubmitTerms}>
          <div data-key='backupTerms'>{locale.backupIntroTerms}</div>
        </Checkbox>
      </TermsRow>
      <NavButton disabled={!isBackupTermsAccepted} buttonType='primary' text={locale.buttonContinue} onSubmit={onSubmit} />
      <SkipButton onClick={onCancel}>{isOnboarding ? locale.backupButtonSkip : locale.backupButtonCancel}</SkipButton>
    </StyledWrapper>
  )
}

export default OnboardingRecovery
