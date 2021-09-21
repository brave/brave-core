import * as React from 'react'
import {
  OnboardingWelcome,
  OnboardingCreatePassword,
  OnboardingImportMetaMaskOrLegacy
} from '../../components/desktop'
import { WalletOnboardingSteps } from '../../constants/types'
import { BackButton } from '../../components/shared'
import BackupWallet from './backup-wallet'

export interface Props {
  recoveryPhrase: string[]
  metaMaskWalletDetected: boolean
  braveLegacyWalletDetected: boolean
  hasImportError: boolean
  onSetImportError: (hasError: boolean) => void
  onPasswordProvided: (password: string) => void
  onImportMetaMask: (password: string) => void
  onImportBraveLegacy: (password: string) => void
  onSubmit: (recoveryVerified: boolean) => void
  onShowRestore: () => void
}

function Onboarding (props: Props) {
  const {
    recoveryPhrase,
    metaMaskWalletDetected,
    braveLegacyWalletDetected,
    hasImportError,
    onSetImportError,
    onPasswordProvided,
    onSubmit,
    onShowRestore,
    onImportMetaMask,
    onImportBraveLegacy
  } = props
  const [onboardingStep, setOnboardingStep] = React.useState<WalletOnboardingSteps>(WalletOnboardingSteps.OnboardingWelcome)
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')

  const nextStep = () => {
    if (onboardingStep === WalletOnboardingSteps.OnboardingWelcome && braveLegacyWalletDetected) {
      setOnboardingStep(WalletOnboardingSteps.OnboardingImportBraveLegacy)
      return
    }
    if (onboardingStep === WalletOnboardingSteps.OnboardingBackupWallet) {
      onSubmit(true)
      return
    }
    if (onboardingStep === WalletOnboardingSteps.OnboardingCreatePassword) {
      onPasswordProvided(password)
    }
    setOnboardingStep(onboardingStep + 1)
  }

  const onBack = () => {
    if (onboardingStep === WalletOnboardingSteps.OnboardingImportBraveLegacy
      || onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask) {
      setOnboardingStep(WalletOnboardingSteps.OnboardingWelcome)
      setPassword('')
      return
    }
    setOnboardingStep(onboardingStep - 1)
  }

  const onSkipBackup = () => {
    onSubmit(false)
  }

  const handlePasswordChanged = (value: string) => {
    if (hasImportError) {
      onSetImportError(false)
    }
    setPassword(value)
  }

  const handleConfirmPasswordChanged = (value: string) => {
    setConfirmedPassword(value)
  }

  const onClickImportMetaMask = () => {
    setOnboardingStep(WalletOnboardingSteps.OnboardingImportMetaMask)
  }

  const onImport = () => {
    if (onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask) {
      onImportMetaMask(password)
    } else {
      onImportBraveLegacy(password)
    }
  }

  const showBackButton = React.useMemo(() => {
    if (
      onboardingStep === WalletOnboardingSteps.OnboardingCreatePassword
      || onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask
      || onboardingStep === WalletOnboardingSteps.OnboardingImportBraveLegacy
    ) {
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

  const startNormalOnboarding = () => {
    setOnboardingStep(WalletOnboardingSteps.OnboardingCreatePassword)
  }

  const isImporting = onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask
    || onboardingStep === WalletOnboardingSteps.OnboardingImportBraveLegacy

  return (
    <>
      {showBackButton &&
        <BackButton onSubmit={onBack} />
      }
      <>
        {onboardingStep === WalletOnboardingSteps.OnboardingWelcome &&
          <OnboardingWelcome
            onRestore={onShowRestore}
            onSetup={nextStep}
            onClickImportMetaMask={onClickImportMetaMask}
            metaMaskWalletDetected={metaMaskWalletDetected}
          />
        }
        {onboardingStep === WalletOnboardingSteps.OnboardingCreatePassword &&
          <OnboardingCreatePassword
            onSubmit={nextStep}
            onPasswordChanged={handlePasswordChanged}
            onConfirmPasswordChanged={handleConfirmPasswordChanged}
            disabled={checkConfirmedPassword || checkPassword || password === '' || confirmedPassword === ''}
            hasPasswordError={checkPassword}
            hasConfirmPasswordError={checkConfirmedPassword}
          />
        }
        {onboardingStep === WalletOnboardingSteps.OnboardingBackupWallet &&
          <BackupWallet
            isOnboarding={true}
            onCancel={onSkipBackup}
            onSubmit={nextStep}
            onBack={onBack}
            recoveryPhrase={recoveryPhrase}
          />
        }
        {isImporting &&
          <OnboardingImportMetaMaskOrLegacy
            onSubmit={onImport}
            onPasswordChanged={handlePasswordChanged}
            disabled={password === ''}
            onboardingStep={onboardingStep}
            hasImportError={hasImportError}
            onClickLost={startNormalOnboarding}
          />
        }
      </>
    </>
  )
}

export default Onboarding
