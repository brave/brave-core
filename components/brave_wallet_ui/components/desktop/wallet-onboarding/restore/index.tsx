import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  RecoveryPhraseInput,
  ErrorText,
  CheckboxRow,
  LegacyCheckboxRow,
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
  onRestore: (phrase: string, password: string, isLegacy: boolean) => void
  hasRestoreError: boolean
}

function OnboardingRestore (props: Props) {
  const {
    onRestore,
    toggleShowRestore,
    hasRestoreError
  } = props
  const [showRecoveryPhrase, setShowRecoveryPhrase] = React.useState<boolean>(false)
  const [isLegacyWallet, setIsLegacyWallet] = React.useState<boolean>(false)
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')
  const [recoveryPhrase, setRecoveryPhrase] = React.useState<string>('')

  const onBack = () => {
    toggleShowRestore()
    setRecoveryPhrase('')
  }

  const onSubmitRestore = () => {
    // added an additional trim here in case the phrase length is
    // 12, 15, 18 or 21 long and has a space at the end.
    onRestore(recoveryPhrase.trimEnd(), password, isLegacyWallet)
  }

  const handlePasswordChanged = (value: string) => {
    setPassword(value)
  }

  const handleConfirmPasswordChanged = (value: string) => {
    setConfirmedPassword(value)
  }

  const handleRecoveryPhraseChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    const value = event.target.value

    // This prevents there from being a space at the begining of the phrase.
    const removeBegginingWhiteSpace = value.trimStart()

    // This Prevents there from being more than one space between words.
    const removedDoubleSpaces = removeBegginingWhiteSpace.replace(/ +(?= )/g, '')

    // Although the above removes double spaces, it is initialy recognized as a
    // a double-space before it is removed and macOS automatically replaces double-spaces with a period.
    const removePeriod = removedDoubleSpaces.replace(/['/.']/g, '')

    // This prevents an extra space at the end of a 24 word phrase.
    if (recoveryPhrase.split(' ').length === 24) {
      setRecoveryPhrase(removePeriod.trimEnd())
    } else {
      setRecoveryPhrase(removePeriod)
    }
  }

  const isValidRecoveryPhrase = React.useMemo(() => {
    if (recoveryPhrase.trim().split(/\s+/g).length >= 12) {
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

  const onSetIsLegacyWallet = (key: string, selected: boolean) => {
    if (key === 'isLegacy') {
      setIsLegacyWallet(selected)
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
          {recoveryPhrase.split(' ').length === 24 &&
            <LegacyCheckboxRow>
              <Checkbox value={{ isLegacy: isLegacyWallet }} onChange={onSetIsLegacyWallet}>
                <div data-key='isLegacy'>{locale.restoreLegacyCheckBox}</div>
              </Checkbox>
            </LegacyCheckboxRow>
          }
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
