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
import { getLocale } from '../../../../../common/locale'
import { Checkbox } from 'brave-ui'

export interface Props {
  checkIsStrongPassword: (value: string) => Promise<boolean>
  toggleShowRestore: () => void
  onRestore: (phrase: string, password: string, isLegacy: boolean) => void
  hasRestoreError: boolean
}

function OnboardingRestore (props: Props) {
  const {
    onRestore,
    toggleShowRestore,
    checkIsStrongPassword,
    hasRestoreError
  } = props
  const [showRecoveryPhrase, setShowRecoveryPhrase] = React.useState<boolean>(false)
  const [isLegacyWallet, setIsLegacyWallet] = React.useState<boolean>(false)
  const [isStrongPassword, setIsStrongPassword] = React.useState<boolean>(false)
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

  const handlePasswordChanged = async (value: string) => {
    setPassword(value)
    const isStrong = await checkIsStrongPassword(value)
    setIsStrongPassword(isStrong)
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
    if (password === '') {
      return false
    }
    return !isStrongPassword
  }, [password, isStrongPassword])

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
        <Title>{getLocale('braveWalletRestoreTite')}</Title>
        <Description>{getLocale('braveWalletRestoreDescription')}</Description>
        <FormWrapper>
          <RecoveryPhraseInput
            autoFocus={true}
            placeholder={getLocale('braveWalletRestorePlaceholder')}
            onChange={handleRecoveryPhraseChanged}
            value={recoveryPhrase}
            type={showRecoveryPhrase ? 'text' : 'password'}
            autoComplete='off'
          />
          {hasRestoreError && <ErrorText>{getLocale('braveWalletRestoreError')}</ErrorText>}
          {recoveryPhrase.split(' ').length === 24 &&
            <LegacyCheckboxRow>
              <Checkbox value={{ isLegacy: isLegacyWallet }} onChange={onSetIsLegacyWallet}>
                <div data-key='isLegacy'>{getLocale('braveWalletRestoreLegacyCheckBox')}</div>
              </Checkbox>
            </LegacyCheckboxRow>
          }
          <CheckboxRow>
            <Checkbox value={{ showPhrase: showRecoveryPhrase }} onChange={onShowRecoveryPhrase}>
              <div data-key='showPhrase'>{getLocale('braveWalletRestoreShowPhrase')}</div>
            </Checkbox>
          </CheckboxRow>
          <FormText>{getLocale('braveWalletRestoreFormText')}</FormText>
          <Description textAlign='left'>{getLocale('braveWalletCreatePasswordDescription')}</Description>
          <InputColumn>
            <PasswordInput
              placeholder={getLocale('braveWalletCreatePasswordInput')}
              onChange={handlePasswordChanged}
              hasError={checkPassword}
              error={getLocale('braveWalletCreatePasswordError')}
              onKeyDown={handleKeyDown}
            />
            <PasswordInput
              placeholder={getLocale('braveWalletConfirmPasswordInput')}
              onChange={handleConfirmPasswordChanged}
              hasError={checkConfirmedPassword}
              error={getLocale('braveWalletConfirmPasswordError')}
              onKeyDown={handleKeyDown}
            />
          </InputColumn>
        </FormWrapper>
        <NavButton
          disabled={isDisabled}
          buttonType='primary'
          text={getLocale('braveWalletWelcomeRestoreButton')}
          onSubmit={onSubmitRestore}
        />
      </StyledWrapper>
    </>
  )
}

export default OnboardingRestore
