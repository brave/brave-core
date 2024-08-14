// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// constants
import { BraveWallet } from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  useCreateWalletMutation,
  useReportOnboardingActionMutation,
  useSetAutoLockMinutesMutation
} from '../../../../common/slices/api.slice'
import {
  useSafeWalletSelector //
} from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'
import { autoLockOptions } from '../../../../options/auto_lock_options'

// components
import {
  NewPasswordValues //
} from '../../../../components/shared/password-input/new-password-input'
import {
  OnboardingCreatingWallet //
} from '../creating_wallet/onboarding_creating_wallet'
import {
  OnboardingContentLayout //
} from '../components/onboarding_content_layout/content_layout'
import { CreatePassword } from './components/create_password'
import { AutoLockSettings } from '../components/auto_lock_settings/auto_lock_settings'

// styles
import { Column } from '../../../../components/shared/style'
import { ContinueButton, NextButtonRow } from '../onboarding.style'

interface OnboardingCreatePasswordProps {
  onWalletCreated: () => void
}

export const OnboardingCreatePassword = ({
  onWalletCreated
}: OnboardingCreatePasswordProps) => {
  // redux
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)

  // state
  const [isValid, setIsValid] = React.useState(false)
  const [password, setPassword] = React.useState('')
  const [autoLockDuration, setAutoLockDuration] = React.useState(
    autoLockOptions[0].minutes
  )

  // mutations
  const [createWallet, { isLoading: isCreatingWallet }] =
    useCreateWalletMutation()
  const [report] = useReportOnboardingActionMutation()
  const [setAutoLockMinutes] = useSetAutoLockMinutesMutation()

  // methods
  const nextStep = React.useCallback(async () => {
    if (!isValid) {
      return
    }
    // Note: intentionally not using unwrapped value
    // results are returned before other redux actions complete
    await createWallet({ password }).unwrap()
  }, [isValid, createWallet, password])

  const handlePasswordChange = React.useCallback(
    ({ isValid, password }: NewPasswordValues) => {
      setPassword(password)
      setIsValid(isValid)
    },
    []
  )

  const onAutoLockDurationChange = async (autoLockDuration: number) => {
    setAutoLockDuration(autoLockDuration)
    await setAutoLockMinutes(autoLockDuration)
  }

  // effects
  React.useEffect(() => {
    report(BraveWallet.OnboardingAction.LegalAndPassword)
  }, [report])

  React.useEffect(() => {
    // wait for redux before redirecting
    // otherwise, the restricted routes in the router will not be available
    if (!isCreatingWallet && isWalletCreated) {
      onWalletCreated()
    }
  }, [isWalletCreated, onWalletCreated, isCreatingWallet])

  if (isCreatingWallet) {
    return <OnboardingCreatingWallet />
  }

  // render
  return (
    <OnboardingContentLayout
      title={getLocale('braveWalletCreatePasswordTitle')}
      subTitle={getLocale('braveWalletCreatePasswordDescription')}
    >
      <Column
        width='100%'
        justifyContent='center'
        alignItems='center'
        margin='36px 0 24px'
        gap='68px'
      >
        <CreatePassword
          onPasswordChange={handlePasswordChange}
          onSubmit={nextStep}
        />

        <AutoLockSettings
          options={autoLockOptions}
          value={autoLockDuration}
          onChange={onAutoLockDurationChange}
        />
      </Column>

      <NextButtonRow>
        <ContinueButton
          onClick={nextStep}
          isDisabled={!isValid}
        >
          {getLocale('braveWalletButtonContinue')}
        </ContinueButton>
      </NextButtonRow>
    </OnboardingContentLayout>
  )
}

export default OnboardingCreatePassword
