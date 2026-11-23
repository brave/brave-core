// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { RecoveryPhraseLengths } from '$wallet/constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'
import { autoLockOptions } from '$wallet/options/auto_lock_options'

// Hooks
import {
  useSafeWalletSelector, //
} from '$wallet/common/hooks/use-safe-selector'
import { WalletSelectors } from '$wallet/common/selectors'
import {
  useCreateWalletMutation,
  useSetAutoLockMinutesMutation,
} from '$wallet/common/slices/api.slice'

// Components
import {
  NewPasswordValues, //
} from '$wallet/components/shared/password-input/new-password-input'
import {
  OnboardingCreatingWallet, //
} from '$wallet/page/screens/onboarding/creating_wallet/onboarding_creating_wallet'
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/onboarding_content_layout'
import {
  CreatePassword, //
} from '$wallet/page/screens/onboarding/create_password/components/create_password'
import {
  AutoLockSettings, //
} from '$wallet/page/screens/onboarding/components/auto_lock_settings/auto_lock_settings'

// Styled Components
import { Description } from './onboarding_create_password.style'
import { Column, Row, Text } from '$wallet/components/shared/style'
import { Button } from '../onboarding.style'

interface OnboardingCreatePasswordProps {
  recoveryPhraseLength?: RecoveryPhraseLengths
  onWalletCreated: () => void
}

export const OnboardingCreatePassword = ({
  recoveryPhraseLength,
  onWalletCreated,
}: OnboardingCreatePasswordProps) => {
  // Redux
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)

  // State
  const [isValid, setIsValid] = React.useState(false)
  const [password, setPassword] = React.useState('')
  const [autoLockDuration, setAutoLockDuration] = React.useState(
    autoLockOptions[0].minutes,
  )

  // Mutations
  const [createWallet, { isLoading: isCreatingWallet }] =
    useCreateWalletMutation()
  const [setAutoLockMinutes] = useSetAutoLockMinutesMutation()

  // Methods
  const nextStep = React.useCallback(async () => {
    if (!isValid) {
      return
    }
    // Note: intentionally not using unwrapped value
    // results are returned before other redux actions complete
    await createWallet({ password, recoveryPhraseLength }).unwrap()
  }, [isValid, createWallet, password, recoveryPhraseLength])

  const handlePasswordChange = React.useCallback(
    ({ isValid, password }: NewPasswordValues) => {
      setPassword(password)
      setIsValid(isValid)
    },
    [],
  )

  const onAutoLockDurationChange = async (autoLockDuration: number) => {
    setAutoLockDuration(autoLockDuration)
    await setAutoLockMinutes(autoLockDuration)
  }

  // Effects
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

  return (
    <OnboardingContentLayout>
      <Column
        width='100%'
        gap='4px'
        margin='0 0 32px 0'
      >
        <Text
          variant='heading.display3'
          textColor='primary'
        >
          {getLocale(S.BRAVE_WALLET_CREATE_PASSWORD_TITLE)}
        </Text>
        <Description
          variant='large.regular'
          textColor='secondary'
        >
          {getLocale(S.BRAVE_WALLET_CREATE_PASSWORD_DESCRIPTION)}
        </Description>
      </Column>
      <Column
        width='100%'
        justifyContent='center'
        alignItems='center'
        margin='0 0 20px 0'
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

      <Row>
        <Button
          onClick={nextStep}
          isDisabled={!isValid}
          size='large'
        >
          {getLocale(S.BRAVE_WALLET_BUTTON_CONTINUE)}
        </Button>
      </Row>
    </OnboardingContentLayout>
  )
}
