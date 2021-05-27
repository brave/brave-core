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
}

function Onboarding (props: Props) {
  const { recoveryPhrase, onPasswordProvided, onSubmit } = props
  const [onboardingStep, setOnboardingStep] = React.useState<number>(0)
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')

  const onRestore = () => {
    alert('Start Restore Process')
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

  const passwordsMatch = React.useMemo(() => {
    if (password === '' || confirmedPassword === '') {
      return true
    } else {
      return password !== confirmedPassword
    }
  }, [password, confirmedPassword])

  return (
    <>
      {onboardingStep === 1 &&
        <BackButton onSubmit={onBack} />
      }
      {onboardingStep === 0 &&
        <OnboardingWelcome
          onRestore={onRestore}
          onSetup={nextStep}
        />
      }
      {onboardingStep === 1 &&
        <OnboardingCreatePassword
          onSubmit={nextStep}
          onPasswordChanged={handlePasswordChanged}
          onConfirmPasswordChanged={handleConfirmPasswordChanged}
          disabled={passwordsMatch}
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
  )
}

export default Onboarding
