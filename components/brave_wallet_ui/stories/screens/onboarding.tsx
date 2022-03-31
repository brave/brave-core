import * as React from 'react'
import {
  OnboardingWelcome,
  OnboardingCreatePassword,
  OnboardingImportMetaMaskOrLegacy
} from '../../components/desktop'
import {
  WalletOnboardingSteps,
  ImportWalletError
} from '../../constants/types'
import { BackButton } from '../../components/shared'
import BackupWallet from './backup-wallet'
import { OnboardingWrapper } from '../style'

export interface Props {
  recoveryPhrase: string[]
  isMetaMaskInitialized: boolean
  isCryptoWalletsInitialized: boolean
  importError: ImportWalletError
  checkIsStrongPassword: (value: string) => Promise<boolean>
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
    isMetaMaskInitialized,
    isCryptoWalletsInitialized,
    importError,
    checkIsStrongPassword,
    onSetImportError,
    onPasswordProvided,
    onSubmit,
    onShowRestore,
    onImportMetaMask,
    onImportCryptoWallets
  } = props
  const [onboardingStep, setOnboardingStep] = React.useState<WalletOnboardingSteps>(WalletOnboardingSteps.OnboardingWelcome)
  const [isStrongPassword, setIsStrongPassword] = React.useState<boolean>(false)
  const [isStrongImportPassword, setIsStrongImportPassword] = React.useState<boolean>(false)
  const [importPassword, setImportPassword] = React.useState<string>('')
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')
  const [useSamePassword, setUseSamePassword] = React.useState<boolean>(false)
  const [needsNewPassword, setNeedsNewPassword] = React.useState<boolean>(false)
  const [useSamePasswordVerified, setUseSamePasswordVerified] = React.useState<boolean>(false)

  React.useMemo(() => {
    if (importError.hasError) {
      setPassword('')
      setConfirmedPassword('')
      setUseSamePassword(false)
      setNeedsNewPassword(false)
      setUseSamePasswordVerified(false)
    }
  }, [importError])

  const nextStep = () => {
    if (onboardingStep === WalletOnboardingSteps.OnboardingWelcome && isCryptoWalletsInitialized) {
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
    if (onboardingStep === WalletOnboardingSteps.OnboardingImportCryptoWallets ||
      onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask) {
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

  const handleImportPasswordChanged = async (value: string) => {
    if (importError.hasError) {
      onSetImportError(false)
    }
    if (needsNewPassword || useSamePasswordVerified) {
      setNeedsNewPassword(false)
      setUseSamePassword(false)
    }
    setImportPassword(value)
    const isStrong = await checkIsStrongPassword(value)
    setIsStrongImportPassword(isStrong)
  }

  const handlePasswordChanged = async (value: string) => {
    setPassword(value)
    const isStrong = await checkIsStrongPassword(value)
    setIsStrongPassword(isStrong)
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
      onImportCryptoWallets(importPassword, confirmedPassword)
    }
  }
  const onUseSamePassword = (selected: boolean) => {
    setUseSamePassword(selected)
  }

  const showBackButton = React.useMemo(() => {
    if (
      onboardingStep === WalletOnboardingSteps.OnboardingCreatePassword ||
      onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask ||
      onboardingStep === WalletOnboardingSteps.OnboardingImportCryptoWallets
    ) {
      return true
    } else {
      return false
    }
  }, [onboardingStep])

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

  const startNormalOnboarding = () => {
    setOnboardingStep(WalletOnboardingSteps.OnboardingCreatePassword)
  }

  React.useEffect(() => {
    if (useSamePassword) {
      handlePasswordChanged(importPassword)
      setConfirmedPassword(importPassword)
      if (!isStrongImportPassword) {
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
      setIsStrongPassword(false)
    }
  }, [useSamePassword])

  const isImporting = onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask ||
    onboardingStep === WalletOnboardingSteps.OnboardingImportCryptoWallets

  const isCreateWalletDisabled = checkConfirmedPassword || checkPassword || password === '' || confirmedPassword === ''
  const isImportDisabled = isCreateWalletDisabled || importPassword === ''

  return (
    <OnboardingWrapper>
      {showBackButton &&
        <BackButton onSubmit={onBack} />
      }
      <>
        {onboardingStep === WalletOnboardingSteps.OnboardingWelcome &&
          <OnboardingWelcome
            onRestore={onShowRestore}
            onSetup={nextStep}
            onClickImportMetaMask={onClickImportMetaMask}
            isMetaMaskInitialized={isMetaMaskInitialized}
            isCryptoWalletsInitialized={isCryptoWalletsInitialized}
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
            importError={importError}
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
    </OnboardingWrapper>
  )
}

export default Onboarding
