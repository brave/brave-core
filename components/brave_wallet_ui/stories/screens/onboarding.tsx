import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory, useLocation } from 'react-router'

import {
  OnboardingWelcome,
  OnboardingCreatePassword,
  OnboardingImportMetaMaskOrLegacy
} from '../../components/desktop'
import {
  WalletOnboardingSteps,
  PageState,
  WalletRoutes,
  WalletState
} from '../../constants/types'
import { BackButton } from '../../components/shared'
import BackupWallet from './backup-wallet'
import { OnboardingWrapper } from '../style'

import { useLib } from '../../common/hooks/useLib'
import * as WalletPageActions from '../../page/actions/wallet_page_actions'

export const Onboarding = () => {
  // routing
  let history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // custom hooks
  const { isStrongPassword: checkIsStrongPassword } = useLib()

  // redux
  const dispatch = useDispatch()
  const {
    isWalletCreated,
    isWalletLocked
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const {
    mnemonic,
    isCryptoWalletsInitialized,
    isMetaMaskInitialized,
    importWalletError: importError
  } = useSelector(({ page }: { page: PageState }) => page)

  // state
  const [onboardingStep, setOnboardingStep] = React.useState<WalletOnboardingSteps>(WalletOnboardingSteps.OnboardingWelcome)
  const [isStrongPassword, setIsStrongPassword] = React.useState<boolean>(false)
  const [isStrongImportPassword, setIsStrongImportPassword] = React.useState<boolean>(false)
  const [importPassword, setImportPassword] = React.useState<string>('')
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')
  const [useSamePassword, setUseSamePassword] = React.useState<boolean>(false)
  const [needsNewPassword, setNeedsNewPassword] = React.useState<boolean>(false)
  const [useSamePasswordVerified, setUseSamePasswordVerified] = React.useState<boolean>(false)

  // methods
  const onImportMetaMask = React.useCallback((password: string, newPassword: string) => {
    dispatch(WalletPageActions.importFromMetaMask({ password, newPassword }))
  }, [])

  const onImportCryptoWallets = React.useCallback((password: string, newPassword: string) => {
    dispatch(WalletPageActions.importFromCryptoWallets({ password, newPassword }))
  }, [])

  const setImportWalletError = React.useCallback((hasError: boolean) => {
    dispatch(WalletPageActions.setImportWalletError({ hasError }))
  }, [])

  const onShowRestore = React.useCallback(() => {
    if (walletLocation === WalletRoutes.Restore) {
      // If a user has not yet created a wallet and clicks Restore
      // from the panel, we need to route to onboarding if they click back.
      if (!isWalletCreated) {
        history.push(WalletRoutes.Onboarding)
        return
      }
      // If a user has created a wallet and clicks Restore from the panel
      // while the wallet is locked, we need to route to unlock if they click back.
      if (isWalletCreated && isWalletLocked) {
        history.push(WalletRoutes.Unlock)
      }
    } else {
      history.push(WalletRoutes.Restore)
    }
  }, [walletLocation, isWalletCreated, isWalletLocked])

  const onPasswordProvided = React.useCallback((password: string) => {
    dispatch(WalletPageActions.createWallet({ password }))
  }, [])

  const completeWalletSetup = React.useCallback((recoveryVerified: boolean) => {
    if (recoveryVerified) {
      dispatch(WalletPageActions.walletBackupComplete())
    }
    dispatch(WalletPageActions.walletSetupComplete())
  }, [])

  const nextStep = React.useCallback(() => {
    if (onboardingStep === WalletOnboardingSteps.OnboardingWelcome && isCryptoWalletsInitialized) {
      setOnboardingStep(WalletOnboardingSteps.OnboardingImportCryptoWallets)
      return
    }
    if (onboardingStep === WalletOnboardingSteps.OnboardingBackupWallet) {
      completeWalletSetup(true)
      return
    }
    if (onboardingStep === WalletOnboardingSteps.OnboardingCreatePassword) {
      onPasswordProvided(password)
    }
    setOnboardingStep(onboardingStep + 1)
  }, [onboardingStep, completeWalletSetup, onPasswordProvided])

  const onBack = React.useCallback(() => {
    if (
      onboardingStep === WalletOnboardingSteps.OnboardingImportCryptoWallets ||
      onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask
    ) {
      setOnboardingStep(WalletOnboardingSteps.OnboardingWelcome)
      setPassword('')
      setConfirmedPassword('')
      setNeedsNewPassword(false)
      setUseSamePassword(false)
      return
    }
    setOnboardingStep(onboardingStep - 1)
  }, [onboardingStep])

  const onSkipBackup = React.useCallback(() => {
    completeWalletSetup(false)
  }, [completeWalletSetup])

  const handleImportPasswordChanged = React.useCallback(async (value: string) => {
    if (importError.hasError) {
      setImportWalletError(false)
    }
    if (needsNewPassword || useSamePasswordVerified) {
      setNeedsNewPassword(false)
      setUseSamePassword(false)
    }
    setImportPassword(value)
    const isStrong = await checkIsStrongPassword(value)
    setIsStrongImportPassword(isStrong)
  }, [importError, needsNewPassword, useSamePasswordVerified, setImportWalletError])

  const handlePasswordChanged = React.useCallback(async (value: string) => {
    setPassword(value)
    const isStrong = await checkIsStrongPassword(value)
    setIsStrongPassword(isStrong)
  }, [])

  const onClickImportMetaMask = React.useCallback(() => {
    setOnboardingStep(WalletOnboardingSteps.OnboardingImportMetaMask)
  }, [])

  const onImport = React.useCallback(
    () => {
      if (onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask) {
        onImportMetaMask(importPassword, confirmedPassword)
      } else {
        onImportCryptoWallets(importPassword, confirmedPassword)
      }
    },
    [
      onboardingStep,
      importPassword,
      confirmedPassword,
      onImportMetaMask,
      onImportCryptoWallets
    ]
  )

  const startNormalOnboarding = React.useCallback(() => {
    setOnboardingStep(WalletOnboardingSteps.OnboardingCreatePassword)
  }, [])

  // memos
  React.useMemo(() => {
    if (importError.hasError) {
      setPassword('')
      setConfirmedPassword('')
      setUseSamePassword(false)
      setNeedsNewPassword(false)
      setUseSamePasswordVerified(false)
    }
  }, [importError])

  const recoveryPhrase = React.useMemo(() => {
    return (mnemonic || '').split(' ')
  }, [mnemonic])

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

  // effects
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

  // computed
  const isImporting = onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask ||
    onboardingStep === WalletOnboardingSteps.OnboardingImportCryptoWallets

  const isCreateWalletDisabled = checkConfirmedPassword ||
    checkPassword ||
    password === '' ||
    confirmedPassword === ''

  const isImportDisabled = isCreateWalletDisabled || importPassword === ''

  // render
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
            onConfirmPasswordChanged={setConfirmedPassword}
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
            onConfirmPasswordChanged={setConfirmedPassword}
            disabled={isImportDisabled}
            onboardingStep={onboardingStep}
            importError={importError}
            onClickLost={startNormalOnboarding}
            hasPasswordError={checkPassword}
            hasConfirmPasswordError={checkConfirmedPassword}
            needsNewPassword={needsNewPassword}
            useSamePassword={useSamePassword}
            onUseSamePassword={setUseSamePassword}
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
