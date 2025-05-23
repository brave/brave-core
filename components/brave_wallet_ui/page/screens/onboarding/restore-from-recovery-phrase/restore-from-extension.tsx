// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import Button from '@brave/leo/react/button'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  useCheckExternalWalletPasswordMutation,
  useGetWalletsToImportQuery,
  useImportFromMetaMaskMutation,
  useReportOnboardingActionMutation
} from '../../../../common/slices/api.slice'

// types
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// styles
import {
  VerticalSpace,
  ErrorText,
  Row
} from '../../../../components/shared/style'
import { ContinueButton, NextButtonRow } from '../onboarding.style'
import { InputLabel } from './restore-from-recovery-phrase.style'

// components
import {
  LoadingSkeleton //
} from '../../../../components/shared/loading-skeleton/index'
import {
  PasswordInput //
} from '../../../../components/shared/password-input/password-input-v2'
import {
  NewPasswordInput,
  NewPasswordValues
} from '../../../../components/shared/password-input/new-password-input'
import {
  OnboardingCreatingWallet //
} from '../creating_wallet/onboarding_creating_wallet'
import {
  OnboardingContentLayout //
} from '../components/onboarding_content_layout/content_layout'

type RestoreFromExtensionSteps = 'newPassword' | 'currentPassword'

interface Props {
  restoreFrom: 'metamask'
}

export const OnboardingRestoreFromExtension = ({ restoreFrom }: Props) => {
  // routing
  let history = useHistory()

  // queries
  const { isFetching: isCheckingExtensions } = useGetWalletsToImportQuery()

  // mutations
  const [report] = useReportOnboardingActionMutation()

  const [checkExtensionPassword, { isLoading: isCheckingImportPassword }] =
    useCheckExternalWalletPasswordMutation()

  const [
    importFromMetaMask,
    {
      data: importFromMetaMaskResult,
      isLoading: isImportingFromMetaMaskExtension
    }
  ] = useImportFromMetaMaskMutation()

  // computed from mutations
  const isCreatingWallet =
    restoreFrom === 'metamask' && isImportingFromMetaMaskExtension

  // state
  const [isPasswordValid, setIsPasswordValid] = React.useState(false)
  const [password, setPassword] = React.useState('')
  const [extensionPassword, setExtensionPassword] = React.useState('')
  const [extensionPasswordError, setExtensionPasswordError] = React.useState('')
  const [currentStep, setCurrentStep] =
    React.useState<RestoreFromExtensionSteps>('currentPassword')

  const importWalletError =
    extensionPasswordError ||
    (restoreFrom === 'metamask' && importFromMetaMaskResult?.errorMessage)
  // methods
  const checkImportPassword = React.useCallback(async () => {
    const results = await checkExtensionPassword({
      walletType:
          BraveWallet.ExternalWalletType.MetaMask,
      password: extensionPassword
    }).unwrap()
    if (results.errorMessage) {
      setExtensionPasswordError(results.errorMessage)
    } else {
      setExtensionPasswordError('')
      setCurrentStep('newPassword')
    }
  }, [checkExtensionPassword, extensionPassword])

  const restoreWallet = React.useCallback(async () => {
    if (!isPasswordValid) {
      return
    }

    if (restoreFrom === 'metamask') {
      await importFromMetaMask({
        password: extensionPassword,
        newPassword: password
      }).unwrap()
      history.push(WalletRoutes.OnboardingComplete)
    }
  }, [
    isPasswordValid,
    restoreFrom,
    importFromMetaMask,
    extensionPassword,
    password,
    history
  ])

  const handlePasswordChange = React.useCallback(
    ({ isValid, password }: NewPasswordValues) => {
      setPassword(password)
      setIsPasswordValid(isValid)
    },
    []
  )

  const onContinueClicked = React.useCallback(async () => {
    if (currentStep === 'currentPassword' && extensionPassword) {
      return await checkImportPassword()
    }

    if (currentStep === 'newPassword' && isPasswordValid) {
      return await restoreWallet()
    }
  }, [
    currentStep,
    extensionPassword,
    isPasswordValid,
    restoreWallet,
    checkImportPassword
  ])

  const handleKeyDown = React.useCallback(
    (event: React.KeyboardEvent<HTMLElement>) => {
      if (event.key === 'Enter') {
        onContinueClicked()
      }
    },
    [onContinueClicked]
  )

  // memos
  const pageText = React.useMemo(() => {
    switch (currentStep) {
      case 'currentPassword':
        return {
          title: getLocale('braveWalletMetaMaskExtensionDetected'),
          description:
              getLocale('braveWalletMetaMaskExtensionImportDescription')
        }

      case 'newPassword':
        return {
          title: getLocale('braveWalletCreatePasswordTitle'),
          description: getLocale('braveWalletCreatePasswordDescription')
        }

      default:
        return { title: '', description: '' }
    }
  }, [currentStep])

  // effects
  React.useEffect(() => {
    if (currentStep === 'currentPassword') {
      report(BraveWallet.OnboardingAction.LegalAndPassword)
    }
  }, [report, currentStep])

  // render
  if (isCreatingWallet) {
    return <OnboardingCreatingWallet />
  }

  return (
    <OnboardingContentLayout
      title={pageText.title}
      subTitle={pageText.description}
    >
      {isCheckingExtensions && (
        <>
          <LoadingSkeleton
            width={375}
            height={168}
          />
          <VerticalSpace space={'100px'} />
        </>
      )}

      {!isCheckingExtensions && currentStep === 'currentPassword' && (
        <>
          <Row
            justifyContent='flex-start'
            padding='118px 0 0'
            marginBottom={4}
          >
            <InputLabel
              textSize='12px'
              isBold={true}
            >
              {getLocale('braveWalletInputLabelPassword')}
            </InputLabel>
          </Row>
          <PasswordInput
            autoFocus={true}
            onChange={(password) => {
              setExtensionPassword(password)
              setExtensionPasswordError('')
            }}
            value={extensionPassword}
            error={importWalletError || ''}
            hasError={!!importWalletError}
            onKeyDown={handleKeyDown}
            placeholder={
                getLocale('braveWalletMetaMaskPasswordInputPlaceholder')
            }
            name='extensionPassword'
          />

          <VerticalSpace space='88px' />
          <Button
            kind='plain'
            onClick={() => history.push(WalletRoutes.OnboardingRestoreWallet)}
          >
            {getLocale('braveWalletImportWithRecoveryPhrase')}
          </Button>
          <VerticalSpace space='85px' />
        </>
      )}

      {currentStep === 'newPassword' && (
        <>
          <NewPasswordInput
            autoFocus={true}
            onSubmit={restoreWallet}
            onChange={handlePasswordChange}
          />
          {importWalletError && <ErrorText>{importWalletError}</ErrorText>}
        </>
      )}

      {!isCheckingExtensions && (
        <NextButtonRow>
          <ContinueButton
            onClick={onContinueClicked}
            isDisabled={
              isCheckingImportPassword ||
              (currentStep === 'currentPassword' && !extensionPassword) ||
              (currentStep === 'newPassword' && !isPasswordValid)
            }
            isLoading={isCheckingImportPassword}
          >
            {getLocale('braveWalletButtonContinue')}
          </ContinueButton>
        </NextButtonRow>
      )}
    </OnboardingContentLayout>
  )
}
