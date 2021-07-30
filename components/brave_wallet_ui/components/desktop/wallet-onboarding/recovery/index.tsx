import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  TermsRow,
  CopyButton,
  WarningBox,
  WarningText,
  DisclaimerText,
  DisclaimerColumn,
  AlertIcon,
  RecoveryBubble,
  RecoveryBubbleText,
  RecoveryPhraseContainer
} from './style'
import { Tooltip } from '../../../shared'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'
import { Checkbox } from 'brave-ui'

export interface Props {
  onSubmit: () => void
  isRecoveryTermsAccepted: boolean
  onSubmitTerms: (key: string, selected: boolean) => void
  recoverPhrase: string[]
  onCopy: () => void
}

function OnboardingBackup (props: Props) {
  const { onSubmit, isRecoveryTermsAccepted, onSubmitTerms, recoverPhrase, onCopy } = props

  return (
    <StyledWrapper>
      <Title>{locale.recoveryTitle}</Title>
      <Description>{locale.recoveryDescription}</Description>
      <WarningBox>
        <AlertIcon />
        <DisclaimerColumn>
          <DisclaimerText><WarningText>{locale.recoveryWarning1} </WarningText>{locale.recoveryWarning2}</DisclaimerText>
          <DisclaimerText>{locale.recoveryWarning3}</DisclaimerText>
        </DisclaimerColumn>
      </WarningBox>
      <RecoveryPhraseContainer>
        {recoverPhrase.map((word) =>
          <RecoveryBubble key={word}>
            <RecoveryBubbleText>{recoverPhrase.indexOf(word) + 1}. {word}</RecoveryBubbleText>
          </RecoveryBubble>
        )}
      </RecoveryPhraseContainer>
      <Tooltip text={locale.toolTipCopyToClipboard}>
        <CopyButton onClick={onCopy}>{locale.buttonCopy}</CopyButton>
      </Tooltip>
      <TermsRow>
        <Checkbox value={{ backedUp: isRecoveryTermsAccepted }} onChange={onSubmitTerms}>
          <div data-key='backedUp'>{locale.recoveryTerms}</div>
        </Checkbox>
      </TermsRow>
      <NavButton disabled={!isRecoveryTermsAccepted} buttonType='primary' text={locale.buttonContinue} onSubmit={onSubmit} />
    </StyledWrapper>
  )
}

export default OnboardingBackup
