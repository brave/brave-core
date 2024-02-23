// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../../common/locale'
import { WalletRoutes } from '../../../../constants/types'

// components
import { OnboardingContentLayout } from '../components/onboarding-content-layout/onboarding-content-layout'
import { Column, VerticalSpace } from '../../../../components/shared/style'

// styles
import {
  Bold,
  Description,
  HardwareGraphic
} from './import-hardware-wallet-welcome.style'
import { ContinueButton } from '../onboarding.style'

export const OnboardingImportHardwareWalletWelcome = () => {
  const history = useHistory()

  const onClickContinue = () => {
    history.push(WalletRoutes.OnboardingHardwareWalletNetworkSelection)
  }

  return (
    <OnboardingContentLayout title='Connect your hardware wallet'>
      <VerticalSpace space='98px' />
      <HardwareGraphic />
      <VerticalSpace space='40px' />
      <Description>
        Connect your hardware wallet to manage your assets directly from Brave
        Wallet
      </Description>
      <VerticalSpace space='28px' />
      <Description>
        We currently support <Bold>Ledger</Bold> and <Bold>Trezor</Bold>{' '}
        devices.
      </Description>
      <VerticalSpace space='117px' />
      <Column>
        <ContinueButton onClick={onClickContinue}>
          {getLocale('braveWalletButtonContinue')}
        </ContinueButton>
      </Column>
    </OnboardingContentLayout>
  )
}
