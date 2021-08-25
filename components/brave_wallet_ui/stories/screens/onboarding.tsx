import * as React from 'react'
import {
  OnboardingWelcome,
  OnboardingCreatePassword
} from '../../components/desktop'
import { BackButton } from '../../components/shared'
import BackupWallet from './backup-wallet'

export interface Props {
  recoveryPhrase: string[]
  onPasswordProvided: (password: string) => void
  onSubmit: (recoveryVerified: boolean) => void
  onShowRestore: () => void
}

function Onboarding (props: Props) {
  const { recoveryPhrase, onPasswordProvided, onSubmit, onShowRestore } = props
  const [onboardingStep, setOnboardingStep] = React.useState<number>(0)
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')

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
    setOnboardingStep(onboardingStep - 1)
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

  const showBackButton = React.useMemo(() => {
    if (onboardingStep === 1) {
      return true
    } else {
      return false
    }
  }, [onboardingStep])

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
    </>
  )
}

export default Onboarding
