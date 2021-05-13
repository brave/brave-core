import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  IconBackground,
  PageIcon,
  TermsRow
} from './style'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'
import { Checkbox } from 'brave-ui'

export interface Props {
  onSubmit: () => void
  isBackupTermsAccepted: boolean
  onSubmitTerms: (key: string, selected: boolean) => void
}

function OnboardingRecovery (props: Props) {
  const { onSubmit, isBackupTermsAccepted, onSubmitTerms } = props

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
    </StyledWrapper>
  )
}

export default OnboardingRecovery
