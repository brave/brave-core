import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  RecoveryBubble,
  RecoveryBubbleText,
  RecoveryPhraseContainer,
  SelectedPhraseContainer,
  SelectedBubble,
  SelectedBubbleText,
  ErrorText,
  ErrorContainer
} from './style'
import { NavButton } from '../../../extension'
import { RecoveryObject } from '../../../../constants/types'
import locale from '../../../../constants/locale'

export interface Props {
  onSubmit: () => void
  recoveryPhrase: RecoveryObject[]
  sortedPhrase: RecoveryObject[]
  selectWord: (word: RecoveryObject) => void
  unSelectWord: (word: RecoveryObject) => void
  hasVerifyError: boolean
}

function OnboardingVerify (props: Props) {
  const { onSubmit, recoveryPhrase, selectWord, unSelectWord, sortedPhrase, hasVerifyError } = props

  const addWord = (word: RecoveryObject) => () => {
    selectWord(word)
  }

  const removeWord = (word: RecoveryObject) => () => {
    unSelectWord(word)
  }

  return (
    <StyledWrapper>
      <Title>{locale.verifyRecoveryTitle}</Title>
      <Description>{locale.verifyRecoveryDescription}</Description>
      <SelectedPhraseContainer error={hasVerifyError}>
        {sortedPhrase.map((word, index) =>
          <SelectedBubble
            key={word.id}
            onClick={removeWord(word)}
          >
            <SelectedBubbleText>{index + 1}. {word.value}</SelectedBubbleText>
          </SelectedBubble>
        )}
        {hasVerifyError &&
          <ErrorContainer>
            <ErrorText>{locale.verifyError}</ErrorText>
          </ErrorContainer>
        }
      </SelectedPhraseContainer>
      <RecoveryPhraseContainer>
        {recoveryPhrase.map((word) =>
          <RecoveryBubble
            key={word.id}
            onClick={addWord(word)}
            disabled={sortedPhrase.includes(word)}
            isSelected={sortedPhrase.includes(word)}
          >
            <RecoveryBubbleText isSelected={sortedPhrase.includes(word)}>{word.value}</RecoveryBubbleText>
          </RecoveryBubble>
        )}
      </RecoveryPhraseContainer>
      <NavButton disabled={sortedPhrase.length !== 12} buttonType='primary' text={locale.buttonVerify} onSubmit={onSubmit} />
    </StyledWrapper>
  )
}

export default OnboardingVerify
