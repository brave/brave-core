// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useDispatch } from 'react-redux'

// utils
import { getLocale } from '../../../../../common/locale'
import { WalletPageActions } from '../../../actions'

// routes
import { WalletRoutes } from '../../../../constants/types'

// hooks
import { useApiProxy } from '../../../../common/hooks/use-api-proxy'
import { usePasswordAttempts } from '../../../../common/hooks/use-password-attempts'

// components
import { PasswordInput } from '../../../../components/shared'
import { NavButton } from '../../../../components/extension'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { StepsNavigation } from '../../../../components/desktop/steps-navigation/steps-navigation'

// style
import { VerticalSpacer } from '../../../../components/shared/style'
import {
  StyledWrapper,
  Title,
  Description,
  NextButtonRow,
  MainWrapper
} from '../../onboarding/onboarding.style'

export const BackupEnterPassword: React.FC = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // context
  const { keyringService } = useApiProxy()

  // custom hooks
  const { attemptPasswordEntry } = usePasswordAttempts()

  // state
  const [password, setPassword] = React.useState('')
  const [isCorrectPassword, setIsCorrectPassword] = React.useState<boolean>(true)

  // memos

  // methods
  const onSubmit = async () => {
    if (!password) { // require password to continue
      return
    }

    // entered password must be correct
    const isPasswordValid = await attemptPasswordEntry(password)

    if (!isPasswordValid) {
      setIsCorrectPassword(isPasswordValid) // set or clear error
      return // need valid password to continue
    }

    // clear entered password & error
    setPassword('')
    setIsCorrectPassword(true)

    const { mnemonic } = await keyringService.getMnemonicForDefaultKeyring(password)
    if (mnemonic) {
      dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic }))
      history.push(WalletRoutes.BackupExplainRecoveryPhrase)
    }
  }

  const handlePasswordKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      onSubmit()
    }
  }

  const onPasswordChange = function (value: string): void {
    setIsCorrectPassword(true) // clear error
    setPassword(value)
  }

  // render
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <StepsNavigation
            currentStep={WalletRoutes.OnboardingExplainRecoveryPhrase}
            steps={[]}
          />

          <div>
            <Title>
              {getLocale('braveWalletEnterAPassswordToContinue')}
            </Title>
            <Description>
              {getLocale('braveWalletEnterYourPasswordToStartBackup')}
            </Description>

            <VerticalSpacer space={56} />

            <PasswordInput
              placeholder={getLocale('braveWalletEnterYourPassword')}
              label={getLocale('braveWalletInputLabelPassword')}
              onChange={onPasswordChange}
              onKeyDown={handlePasswordKeyDown}
              hasError={!!password && !isCorrectPassword}
              value={password}
              error={getLocale('braveWalletLockScreenError')}
              autoFocus
              name='password'
            />
          </div>

          <VerticalSpacer space={194} />

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonContinue')}
              disabled={!password || !isCorrectPassword}
              onSubmit={onSubmit}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}

export default BackupEnterPassword
