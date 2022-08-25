// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory } from 'react-router'

// utils
import { copyToClipboard } from '../../../utils/copy-to-clipboard'

// actions
import { WalletPageActions } from '../../actions'

// types
import { PageState, WalletRoutes } from '../../../constants/types'

// components
import { BackButton } from '../../../components/shared'
import { BackupWalletIntroStep } from './intro/backup-wallet-intro'
import { BackupWalletRecoveryStep } from './recovery/backup-wallet-recovery'
import { BackupWalletVerifyStep } from './verify/backup-wallet-verify'

export interface Props {
  onCancel: () => void
  isOnboarding: boolean
}

const recoverPhraseCopiedTimeout = 5000 // 5s

export const BackupWallet = (props: Props) => {
  const { isOnboarding, onCancel } = props

  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const mnemonic = useSelector(({ page }: { page: PageState }) => (page?.mnemonic || ''))
  const recoveryPhrase = React.useMemo(() => (mnemonic || '').split(' '), [mnemonic])

  // state
  const [backupStep, setBackupStep] = React.useState<number>(0)
  const [backupTerms, setBackupTerms] = React.useState<boolean>(false)
  const [backedUp, setBackedUp] = React.useState<boolean>(false)
  const [isRecoverPhraseCopied, setIsRecoverPhraseCopied] = React.useState(false)

  // methods
  const onSubmit = React.useCallback(() => {
    dispatch(WalletPageActions.walletBackupComplete())
    dispatch(WalletPageActions.walletSetupComplete(true))

    if (isOnboarding) {
      history.push(WalletRoutes.Portfolio)
    } else {
      history.goBack()
    }
  }, [isOnboarding])

  const nextStep = React.useCallback(() => {
    if (backupStep === 2) {
      dispatch(WalletPageActions.showRecoveryPhrase({ show: false }))
      onSubmit()
      return
    }
    setBackupStep(backupStep + 1)
  }, [backupStep, onSubmit])

  const onGoBack = React.useCallback(() => {
    if (isOnboarding && backupStep === 0) {
      history.goBack()
    } else {
      setBackupStep(backupStep - 1)
    }
  }, [isOnboarding, backupStep])

  const checkedBox = React.useCallback((key: string, selected: boolean) => {
    if (key === 'backupTerms') {
      setBackupTerms(selected)
    }
    if (key === 'backedUp') {
      setBackedUp(selected)
    }
  }, [])

  const onCopyToClipboard = React.useCallback(async () => {
    await copyToClipboard(recoveryPhrase.join(' '))
    setIsRecoverPhraseCopied(true)
  }, [recoveryPhrase])

  // effects
  React.useEffect(() => {
    dispatch(WalletPageActions.showRecoveryPhrase({ show: false }))
  }, [])

  React.useEffect(() => {
    if (isRecoverPhraseCopied) {
      const timer = setTimeout(() => {
        setIsRecoverPhraseCopied(false)
      }, recoverPhraseCopiedTimeout)
      return () => clearTimeout(timer)
    }
    return () => {}
  }, [isRecoverPhraseCopied])

  // render
  return (
    <>
      {backupStep !== 0 &&
        <BackButton onSubmit={onGoBack} />
      }
      {backupStep === 0 &&
        <BackupWalletIntroStep
          onSubmit={nextStep}
          onSubmitTerms={checkedBox}
          onCancel={onCancel}
          isBackupTermsAccepted={backupTerms}
          isOnboarding={isOnboarding}
          recoveryPhraseLength={recoveryPhrase.length}
        />
      }
      {backupStep === 1 &&
        <BackupWalletRecoveryStep
          onSubmit={nextStep}
          isRecoveryTermsAccepted={backedUp}
          onSubmitTerms={checkedBox}
          recoverPhrase={recoveryPhrase}
          isRecoverPhraseCopied={isRecoverPhraseCopied}
          onCopy={onCopyToClipboard}
        />
      }
      {backupStep === 2 &&
        <BackupWalletVerifyStep onNextStep={nextStep} />
      }
    </>
  )
}

export default BackupWallet
