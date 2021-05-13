import * as React from 'react'
import {
  WalletSubViewLayout,
  OnboardingWelcome,
  OnboardingBackup,
  OnboardingRecovery,
  OnboardingVerify,
  OnboardingCreatePassword
} from '../../components/desktop'

export interface Props {
  recoveryPhrase: string[]
  onSubmit: () => void
}

function Onboarding (props: Props) {
  const { recoveryPhrase, onSubmit } = props
  const [onboardingStep, setOnboardingStep] = React.useState<number>(0)
  const [backupTerms, setBackupTerms] = React.useState<boolean>(false)
  const [backedUp, setBackedUp] = React.useState<boolean>(false)
  const [sortedPhrase, setSortedPhrase] = React.useState<string[]>([])
  const [verifyError, setVerifyError] = React.useState<boolean>(false)
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')

  const onRestore = () => {
    alert('Start Restore Process')
  }

  const nextStep = () => {
    if (onboardingStep === 4) {
      onSubmit()
    } else {
      setOnboardingStep(onboardingStep + 1)
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

  const selectWord = (word: string) => {
    const newList = [...sortedPhrase, word]
    setSortedPhrase(newList)
    setVerifyError(false)
  }

  const unSelectWord = (word: string) => {
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
    return array
  }, [])

  const showError = () => {
    setVerifyError(true)
    setTimeout(function () { setVerifyError(false) }, 3000)
  }

  const checkPhrase = () => {
    if (sortedPhrase.length === recoveryPhrase.length && sortedPhrase.every((v, i) => v === recoveryPhrase[i])) {
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
    <WalletSubViewLayout>
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
        <OnboardingBackup
          onSubmit={nextStep}
          onSubmitTerms={checkedBox}
          isBackupTermsAccepted={backupTerms}
        />
      }
      {onboardingStep === 3 &&
        <OnboardingRecovery
          onSubmit={nextStep}
          isRecoveryTermsAccepted={backedUp}
          onSubmitTerms={checkedBox}
          recoverPhrase={recoveryPhrase}
          onCopy={copyToClipboard}
        />
      }
      {onboardingStep === 4 &&
        <OnboardingVerify
          onSubmit={checkPhrase}
          recoveryPhrase={shuffledPhrase}
          sortedPhrase={sortedPhrase}
          selectWord={selectWord}
          unSelectWord={unSelectWord}
          hasVerifyError={verifyError}
        />
      }
    </WalletSubViewLayout>
  )
}

export default Onboarding
