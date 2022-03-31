import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  TermsRow,
  SkipButton,
  PageIcon
} from './style'
import { NavButton } from '../../../extension'
import { getLocale } from '../../../../../common/locale'
import { Checkbox } from 'brave-ui'

export interface Props {
  onSubmit: () => void
  isBackupTermsAccepted: boolean
  isOnboarding: boolean
  onSubmitTerms: (key: string, selected: boolean) => void
  onCancel: () => void
  recoveryPhraseLength: number
}

function OnboardingRecovery (props: Props) {
  const { onSubmit, isBackupTermsAccepted, isOnboarding, onSubmitTerms, onCancel, recoveryPhraseLength } = props

  return (
    <StyledWrapper>
      <PageIcon />
      <Title>{getLocale('braveWalletBackupIntroTitle')}</Title>
      <Description>{getLocale('braveWalletBackupIntroDescription').replace('$1', recoveryPhraseLength.toString())}</Description>
      <TermsRow>
        <Checkbox value={{ backupTerms: isBackupTermsAccepted }} onChange={onSubmitTerms}>
          <div data-key='backupTerms'>{getLocale('braveWalletBackupIntroTerms')}</div>
        </Checkbox>
      </TermsRow>
      <NavButton disabled={!isBackupTermsAccepted} buttonType='primary' text={getLocale('braveWalletButtonContinue')} onSubmit={onSubmit} />
      <SkipButton onClick={onCancel}>{isOnboarding ? getLocale('braveWalletBackupButtonSkip') : getLocale('braveWalletBackupButtonCancel')}</SkipButton>
    </StyledWrapper>
  )
}

export default OnboardingRecovery
