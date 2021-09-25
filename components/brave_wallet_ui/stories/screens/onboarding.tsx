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
  onImportMetaMask: (password: string, newPassword: string) => void
  onImportCryptoWallets: (password: string, newPassword: string) => void
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
    onImportCryptoWallets
  } = props
  const [onboardingStep, setOnboardingStep] = React.useState<WalletOnboardingSteps>(WalletOnboardingSteps.OnboardingWelcome)
  const [importPassword, setImportPassword] = React.useState<string>('')
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')
  const [useSamePassword, setUseSamePassword] = React.useState<boolean>(false)
  const [needsNewPassword, setNeedsNewPassword] = React.useState<boolean>(false)
  const [useSamePasswordVerified, setUseSamePasswordVerified] = React.useState<boolean>(false)

  React.useMemo(() => {
    if (hasImportError) {
      setPassword('')
      setConfirmedPassword('')
      setUseSamePassword(false)
      setNeedsNewPassword(false)
      setUseSamePasswordVerified(false)
    }
  }, [hasImportError])

  const nextStep = () => {
    if (onboardingStep === WalletOnboardingSteps.OnboardingWelcome && braveLegacyWalletDetected) {
      setOnboardingStep(WalletOnboardingSteps.OnboardingImportCryptoWallets)
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
    if (onboardingStep === WalletOnboardingSteps.OnboardingImportCryptoWallets
      || onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask) {
      setOnboardingStep(WalletOnboardingSteps.OnboardingWelcome)
      setPassword('')
      setConfirmedPassword('')
      setNeedsNewPassword(false)
      setUseSamePassword(false)
      return
    }
    setOnboardingStep(onboardingStep - 1)
  }

  const onSkipBackup = () => {
    onSubmit(false)
  }

  const handleImportPasswordChanged = (value: string) => {
    if (hasImportError) {
      onSetImportError(false)
    }
    if (needsNewPassword || useSamePasswordVerified) {
      setNeedsNewPassword(false)
      setUseSamePassword(false)
    }
    setImportPassword(value)
  }

  const handlePasswordChanged = (value: string) => {
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
      onImportMetaMask(importPassword, confirmedPassword)
    } else {
      onImportCryptoWallets(password, confirmedPassword)
    }
  }
  const onUseSamePassword = (selected: boolean) => {
    setUseSamePassword(selected)
  }

  const showBackButton = React.useMemo(() => {
    if (
      onboardingStep === WalletOnboardingSteps.OnboardingCreatePassword
      || onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask
      || onboardingStep === WalletOnboardingSteps.OnboardingImportCryptoWallets
    ) {
      return true
    } else {
      return false
    }
  }, [onboardingStep])

  const strongPassword = new RegExp('^(?=.*[0-9])(?=.*[!@#$%^&*])(?=.{7,})')

  const checkPassword = React.useMemo(() => {
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

  React.useMemo(() => {
    if (useSamePassword) {
      setPassword(importPassword)
      setConfirmedPassword(importPassword)
      if (!strongPassword.test(importPassword)) {
        setNeedsNewPassword(true)
        setUseSamePasswordVerified(false)
      } else {
        setNeedsNewPassword(false)
        setUseSamePasswordVerified(true)
      }
    } else {
      setPassword('')
      setConfirmedPassword('')
      setNeedsNewPassword(false)
      setUseSamePasswordVerified(false)
    }
  }, [useSamePassword])

  const isImporting = onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask
    || onboardingStep === WalletOnboardingSteps.OnboardingImportCryptoWallets

  const isCreateWalletDisabled = checkConfirmedPassword || checkPassword || password === '' || confirmedPassword === ''
  const isImportDisabled = isCreateWalletDisabled || importPassword === ''

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
            disabled={isCreateWalletDisabled}
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
            onImportPasswordChanged={handleImportPasswordChanged}
            onPasswordChanged={handlePasswordChanged}
            onConfirmPasswordChanged={handleConfirmPasswordChanged}
            disabled={isImportDisabled}
            onboardingStep={onboardingStep}
            hasImportError={hasImportError}
            onClickLost={startNormalOnboarding}
            hasPasswordError={checkPassword}
            hasConfirmPasswordError={checkConfirmedPassword}
            needsNewPassword={needsNewPassword}
            useSamePassword={useSamePassword}
            onUseSamePassword={onUseSamePassword}
            importPassword={importPassword}
            password={password}
            confirmedPassword={confirmedPassword}
            useSamePasswordVerified={useSamePasswordVerified}
          />
        }
      </>
    </>
  )
}

export default Onboarding
