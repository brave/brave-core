import * as React from 'react'
import {
  OnboardingWelcome,
  OnboardingCreatePassword,
  OnboardingRestore
} from '../../components/desktop'
import { BackButton } from '../../components/shared'
import BackupWallet from './backup-wallet'

export interface Props {
  recoveryPhrase: string[]
  onPasswordProvided: (password: string) => void
  onRestore: (phrase: string, password: string) => void
  hasRestoreError?: boolean
  onSubmit: (recoveryVerified: boolean) => void
}

function Onboarding (props: Props) {
  const { recoveryPhrase, onPasswordProvided, onSubmit, onRestore, hasRestoreError } = props
  const [onboardingStep, setOnboardingStep] = React.useState<number>(0)
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')
  const [restorePhrase, setRestorePhrase] = React.useState<string>('')
  const [isRestoring, setIsRestoring] = React.useState<boolean>(false)

  const onShowRestore = () => {
    setIsRestoring(true)
  }

  const nextStep = () => {
    if (onboardingStep === 2) {
      onSubmit(true)
      return
    }

    if (onboardingStep === 1) {
      onPasswordProvided(password)
    }
    setOnboardingStep(onboardingStep + 1)
  }

  const onBack = () => {
    if (isRestoring) {
      setIsRestoring(false)
      setRestorePhrase('')
      return
    }
    setOnboardingStep(onboardingStep - 1)
  }

  const onSubmitRestore = () => {
    onRestore(restorePhrase, password)
  }

  const onSkipBackup = () => {
    onSubmit(false)
  }

  const handlePasswordChanged = (value: string) => {
    setPassword(value)
  }

  const handleConfirmPasswordChanged = (value: string) => {
    setConfirmedPassword(value)
  }

  const handleRecoveryPhraseChanged = (value: string) => {
    const removeBegginingWhiteSpace = value.trimStart()
    const removedDoubleSpaces = removeBegginingWhiteSpace.replace(/ +(?= )/g, '')
    const removedSpecialCharacters = removedDoubleSpaces.replace(/[^a-zA-Z ]/g, '')
    if (restorePhrase.split(' ').length === 12) {
      setRestorePhrase(removedSpecialCharacters.trimEnd())
    } else {
      setRestorePhrase(removedSpecialCharacters)
    }
  }

  const isValidRecoveryPhrase = React.useMemo(() => {
    const phrase = restorePhrase.split(' ')
    if (phrase.length === 12 && phrase[11] !== '') {
      return false
    } else {
      return true
    }
  }, [restorePhrase])

  const showBackButton = React.useMemo(() => {
    if (onboardingStep === 1 || isRestoring) {
      return true
    } else {
      return false
    }
  }, [onboardingStep, isRestoring])

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

  return (
    <>
      {showBackButton &&
        <BackButton onSubmit={onBack} />
      }
      {isRestoring ? (
        <OnboardingRestore
          onSubmit={onSubmitRestore}
          disabled={isValidRecoveryPhrase || checkConfirmedPassword || checkPassword || password === '' || confirmedPassword === ''}
          onRecoveryPhraseChanged={handleRecoveryPhraseChanged}
          onPasswordChanged={handlePasswordChanged}
          onConfirmPasswordChanged={handleConfirmPasswordChanged}
          recoveryPhrase={restorePhrase}
          hasRestoreError={hasRestoreError}
          hasPasswordError={checkPassword}
          hasConfirmPasswordError={checkConfirmedPassword}
        />
      ) : (
        <>
          {onboardingStep === 0 &&
            <OnboardingWelcome
              onRestore={onShowRestore}
              onSetup={nextStep}
            />
          }
          {onboardingStep === 1 &&
            <OnboardingCreatePassword
              onSubmit={nextStep}
              onPasswordChanged={handlePasswordChanged}
              onConfirmPasswordChanged={handleConfirmPasswordChanged}
              disabled={checkConfirmedPassword || checkPassword || password === '' || confirmedPassword === ''}
              hasPasswordError={checkPassword}
              hasConfirmPasswordError={checkConfirmedPassword}
            />
          }
          {onboardingStep === 2 &&
            <BackupWallet
              isOnboarding={true}
              onCancel={onSkipBackup}
              onSubmit={nextStep}
              onBack={onBack}
              recoveryPhrase={recoveryPhrase}
            />
          }
        </>
      )}
    </>
  )
}

export default Onboarding
