import * as React from 'react'
import {
  OnboardingBackup,
  OnboardingRecovery,
  OnboardingVerify
} from '../../components/desktop'
import { RecoveryObject } from '../../constants/types'
import { BackButton } from '../../components/shared'

export interface Props {
  recoveryPhrase: string[]
  onSubmit: () => void
  onCancel: () => void
  onBack?: () => void
  isOnboarding: boolean
}

function BackupWallet (props: Props) {
  const { recoveryPhrase, isOnboarding, onSubmit, onCancel, onBack } = props
  const [backupStep, setBackupStep] = React.useState<number>(0)
  const [backupTerms, setBackupTerms] = React.useState<boolean>(false)
  const [backedUp, setBackedUp] = React.useState<boolean>(false)
  const [sortedPhrase, setSortedPhrase] = React.useState<RecoveryObject[]>([])
  const [verifyError, setVerifyError] = React.useState<boolean>(false)

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

  const selectWord = (word: RecoveryObject) => {
    const newList = [...sortedPhrase, word]
    setSortedPhrase(newList)
    setVerifyError(false)
  }

  const unSelectWord = (word: RecoveryObject) => {
    const newList = sortedPhrase.filter((key) => key !== word)
    setSortedPhrase(newList)
  }

  const shuffledPhrase = React.useMemo(() => {
    const array = recoveryPhrase.slice().sort()
    for (let i = array.length - 1; i > 0; i--) {
      let j = Math.floor(Math.random() * (i + 1))
      let temp = array[i]
      array[i] = array[j]
      array[j] = temp
    }
    return array.map((str, index) => ({ value: str, id: index + 1 }))
  }, [recoveryPhrase])

  const showError = () => {
    setVerifyError(true)
    setTimeout(function () { setVerifyError(false) }, 3000)
  }

  const checkPhrase = () => {
    if (sortedPhrase.length === recoveryPhrase.length && sortedPhrase.every((v, i) => v.value === recoveryPhrase[i])) {
      nextStep()
    } else {
      setSortedPhrase([])
      showError()
    }
  }

  const copyToClipboard = async () => {
    try {
      await navigator.clipboard.writeText(recoveryPhrase.join(' '))
    } catch (e) {
      console.log(`Could not copy address ${e.toString()}`)
    }
  }

  const showBackButton = React.useMemo(() => {
    if (isOnboarding) {
      return true
    }
    if (!isOnboarding && backupStep !== 0) {
      return true
    }
    return false
  }, [isOnboarding, backupStep])

  return (
    <>
      {showBackButton &&
        <BackButton onSubmit={onGoBack} />
      }
      {backupStep === 0 &&
        <OnboardingBackup
          onSubmit={nextStep}
          onSubmitTerms={checkedBox}
          onCancel={onCancel}
          isBackupTermsAccepted={backupTerms}
          isOnboarding={isOnboarding}
        />
      }
      {backupStep === 1 &&
        <OnboardingRecovery
          onSubmit={nextStep}
          isRecoveryTermsAccepted={backedUp}
          onSubmitTerms={checkedBox}
          recoverPhrase={recoveryPhrase}
          onCopy={copyToClipboard}
        />
      }
      {backupStep === 2 &&
        <OnboardingVerify
          onSubmit={checkPhrase}
          recoveryPhrase={shuffledPhrase}
          sortedPhrase={sortedPhrase}
          selectWord={selectWord}
          unSelectWord={unSelectWord}
          hasVerifyError={verifyError}
        />
      }
    </>
  )
}

export default BackupWallet
