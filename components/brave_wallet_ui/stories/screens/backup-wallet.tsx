import * as React from 'react'
import {
  OnboardingBackup,
  OnboardingRecovery,
  OnboardingVerify
} from '../../components/desktop'

import { BackButton } from '../../components/shared'
import { copyToClipboard } from '../../utils/copy-to-clipboard'

export interface Props {
  recoveryPhrase: string[]
  onSubmit: () => void
  onCancel: () => void
  onBack?: () => void
  isOnboarding: boolean
}

const recoverPhraseCopiedTimeout = 5000 // 5s

export function BackupWallet (props: Props) {
  const { recoveryPhrase, isOnboarding, onSubmit, onCancel, onBack } = props
  const [backupStep, setBackupStep] = React.useState<number>(0)
  const [backupTerms, setBackupTerms] = React.useState<boolean>(false)
  const [backedUp, setBackedUp] = React.useState<boolean>(false)

  const [isRecoverPhraseCopied, setIsRecoverPhraseCopied] = React.useState(false)

  const nextStep = () => {
    if (backupStep === 2) {
      onSubmit()
      return
    }
    setBackupStep(backupStep + 1)
  }

  const onGoBack = () => {
    if (onBack && isOnboarding && backupStep === 0) {
      onBack()
    } else {
      setBackupStep(backupStep - 1)
    }
  }

  const checkedBox = (key: string, selected: boolean) => {
    if (key === 'backupTerms') {
      setBackupTerms(selected)
    }
    if (key === 'backedUp') {
      setBackedUp(selected)
    }
  }

  const onCopyToClipboard = async () => {
    await copyToClipboard(recoveryPhrase.join(' '))
    setIsRecoverPhraseCopied(true)
  }

  React.useEffect(() => {
    if (isRecoverPhraseCopied) {
      const timer = setTimeout(() => {
        setIsRecoverPhraseCopied(false)
      }, recoverPhraseCopiedTimeout)
      return () => clearTimeout(timer)
    }
    return () => {}
  }, [isRecoverPhraseCopied])

  return (
    <>
      {backupStep !== 0 &&
        <BackButton onSubmit={onGoBack} />
      }
      {backupStep === 0 &&
        <OnboardingBackup
          onSubmit={nextStep}
          onSubmitTerms={checkedBox}
          onCancel={onCancel}
          isBackupTermsAccepted={backupTerms}
          isOnboarding={isOnboarding}
          recoveryPhraseLength={recoveryPhrase.length}
        />
      }
      {backupStep === 1 &&
        <OnboardingRecovery
          onSubmit={nextStep}
          isRecoveryTermsAccepted={backedUp}
          onSubmitTerms={checkedBox}
          recoverPhrase={recoveryPhrase}
          isRecoverPhraseCopied={isRecoverPhraseCopied}
          onCopy={onCopyToClipboard}
        />
      }
      {backupStep === 2 &&
        <OnboardingVerify recoveryPhrase={recoveryPhrase} onNextStep={nextStep} />
      }
    </>
  )
}
