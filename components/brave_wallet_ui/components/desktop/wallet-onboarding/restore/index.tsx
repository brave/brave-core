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
import { PasswordInput, BackButton } from '../../../shared'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'
import { Checkbox } from 'brave-ui'

export interface Props {
  toggleShowRestore: () => void
  onRestore: (phrase: string, password: string) => void
  hasRestoreError: boolean
}

function OnboardingRestore (props: Props) {
  const {
    onRestore,
    toggleShowRestore,
    hasRestoreError
  } = props
  const [showRecoveryPhrase, setShowRecoveryPhrase] = React.useState<boolean>(false)
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')
  const [recoveryPhrase, setRecoveryPhrase] = React.useState<string>('')

  const onBack = () => {
    toggleShowRestore()
    setRecoveryPhrase('')
  }

  const onSubmitRestore = () => {
    onRestore(recoveryPhrase, password)
  }

  const handlePasswordChanged = (value: string) => {
    setPassword(value)
  }

  const handleConfirmPasswordChanged = (value: string) => {
    setConfirmedPassword(value)
  }

  const handleRecoveryPhraseChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    const value = event.target.value
    const removeBegginingWhiteSpace = value.trimStart()
    const removedDoubleSpaces = removeBegginingWhiteSpace.replace(/ +(?= )/g, '')
    const removedSpecialCharacters = removedDoubleSpaces.replace(/[^a-zA-Z ]/g, '')
    if (recoveryPhrase.split(' ').length === 12) {
      setRecoveryPhrase(removedSpecialCharacters.trimEnd())
    } else {
      setRecoveryPhrase(removedSpecialCharacters)
    }
  }

  const isValidRecoveryPhrase = React.useMemo(() => {
    const phrase = recoveryPhrase.split(' ')
    if (phrase.length === 12 && phrase[11] !== '') {
      return false
    } else {
      return true
    }
  }, [recoveryPhrase])

  const checkPassword = React.useMemo(() => {
    const strongPassword = new RegExp('^(?=.*[0-9])(?=.*[!@#$%^&*])(?=.{7,})')
    if (password === '') {
      return false
    } else {
      if (!strongPassword.test(password)) {
        return true
      }
      return false
    }
  }, [password])

  const checkConfirmedPassword = React.useMemo(() => {
    if (confirmedPassword === '') {
      return false
    } else {
      return confirmedPassword !== password
    }
  }, [confirmedPassword, password])

  const isDisabled = isValidRecoveryPhrase || checkConfirmedPassword || checkPassword || password === '' || confirmedPassword === ''
  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !isDisabled) {
      onSubmitRestore()
    }
  }

  const onShowRecoveryPhrase = (key: string, selected: boolean) => {
    if (key === 'showPhrase') {
      setShowRecoveryPhrase(selected)
    }
  }

  return (
    <>
      <BackButton onSubmit={onBack} />
      <StyledWrapper>
        <Title>{locale.restoreTite}</Title>
        <Description>{locale.restoreDescription}</Description>
        <FormWrapper>
          <RecoveryPhraseInput
            autoFocus={true}
            placeholder={locale.restorePlaceholder}
            onChange={handleRecoveryPhraseChanged}
            value={recoveryPhrase}
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
              onChange={handlePasswordChanged}
              hasError={checkPassword}
              error={locale.createPasswordError}
            />
            <PasswordInput
              placeholder={locale.createPasswordInput2}
              onChange={handleConfirmPasswordChanged}
              hasError={checkConfirmedPassword}
              error={locale.createPasswordError2}
              onKeyDown={handleKeyDown}
            />
          </InputColumn>
        </FormWrapper>
        <NavButton
          disabled={isDisabled}
          buttonType='primary'
          text={locale.welcomeRestoreButton}
          onSubmit={onSubmitRestore}
        />
      </StyledWrapper>
    </>
  )
}

export default OnboardingRestore
