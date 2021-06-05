import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  RecoveryPhraseInput,
  ErrorText,
  CheckboxRow,
  FormWrapper,
  InputColumn,
  FormText
} from './style'
import { PasswordInput } from '../../../shared'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'
import { Checkbox } from 'brave-ui'

export interface Props {
  onSubmit: () => void
  onRecoveryPhraseChanged: (value: string) => void
  onPasswordChanged: (value: string) => void
  onConfirmPasswordChanged: (value: string) => void
  recoveryPhrase: string
  disabled: boolean
  hasRestoreError?: boolean
  hasPasswordError: boolean
  hasConfirmPasswordError: boolean
}

function OnboardingRestore (props: Props) {
  const {
    onSubmit,
    onRecoveryPhraseChanged,
    onConfirmPasswordChanged,
    onPasswordChanged,
    recoveryPhrase,
    disabled,
    hasRestoreError,
    hasPasswordError,
    hasConfirmPasswordError
  } = props
  const [showRecoveryPhrase, setShowRecoveryPhrase] = React.useState<boolean>(false)

  const inputRecoveryPhrase = (event: React.ChangeEvent<HTMLInputElement>) => {
    onRecoveryPhraseChanged(event.target.value)
  }

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !disabled) {
      onSubmit()
    }
  }

  const onShowRecoveryPhrase = (key: string, selected: boolean) => {
    if (key === 'showPhrase') {
      setShowRecoveryPhrase(selected)
    }
  }

  return (
    <StyledWrapper>
      <Title>{locale.restoreTite}</Title>
      <Description>{locale.restoreDescription}</Description>
      <FormWrapper>
        <RecoveryPhraseInput
          autoFocus={true}
          placeholder={locale.restorePlaceholder}
          onChange={inputRecoveryPhrase}
          value={recoveryPhrase}
          onKeyDown={handleKeyDown}
          type={showRecoveryPhrase ? 'text' : 'password'}
        />
        {hasRestoreError && <ErrorText>{locale.restoreError}</ErrorText>}
        <CheckboxRow>
          <Checkbox value={{ showPhrase: showRecoveryPhrase }} onChange={onShowRecoveryPhrase}>
            <div data-key='showPhrase'>{locale.restoreShowPhrase}</div>
          </Checkbox>
        </CheckboxRow>
        <FormText>{locale.restoreFormText}</FormText>
        <InputColumn>
          <PasswordInput
            placeholder={locale.createPasswordInput}
            onChange={onPasswordChanged}
            hasError={hasPasswordError}
            error={locale.createPasswordError}
          />
          <PasswordInput
            placeholder={locale.createPasswordInput2}
            onChange={onConfirmPasswordChanged}
            hasError={hasConfirmPasswordError}
            error={locale.createPasswordError2}
          />
        </InputColumn>
      </FormWrapper>
      <NavButton disabled={disabled} buttonType='primary' text={locale.welcomeRestoreButton} onSubmit={onSubmit} />
    </StyledWrapper>
  )
}

export default OnboardingRestore
