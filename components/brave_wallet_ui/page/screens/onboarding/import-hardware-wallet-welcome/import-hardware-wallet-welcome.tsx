// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// utils
import { getLocale, splitStringForTag } from '../../../../../common/locale'
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

  const { beforeTag, duringTag, afterTag } = splitStringForTag(
    getLocale('braveWallectConnectHardwareDescription')
  )

  return (
    <OnboardingContentLayout
      title={getLocale('braveWalletConnectHardwareWallet')}
    >
      <VerticalSpace space='98px' />
      <HardwareGraphic />
      <VerticalSpace space='40px' />
      <Description>
        {getLocale('braveWalletImportHardwareWalletDescription')}
      </Description>
      <VerticalSpace space='28px' />
      <Description>
        {beforeTag}
        <Bold>{getLocale('braveWalletConnectHardwareLedger')}</Bold>
        {duringTag}
        <Bold>{getLocale('braveWalletConnectHardwareTrezor')}</Bold>
        {afterTag}
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
