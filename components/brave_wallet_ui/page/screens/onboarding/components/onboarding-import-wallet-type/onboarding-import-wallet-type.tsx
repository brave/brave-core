// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// utils
import { WalletRoutes } from '../../../../../constants/types'

// components
import { OnboardingContentLayout } from '../onboarding-content-layout/onboarding-content-layout'
import {
  VerticalDivider,
  VerticalSpace
} from '../../../../../components/shared/style'
import { ImportTypeAction } from './components/import-type-action'

export const OnboardingImportWalletType = () => {
  const history = useHistory()
  return (
    <OnboardingContentLayout title='Which type of wallet would you like to import?'>
      <VerticalSpace space='92px' />
      <ImportTypeAction
        title='Ethereum/Solana/Filecoin wallet'
        description='Import your seed phrase from an existing wallet'
        icons={[
          'brave-icon-release-color',
          'phantom-color',
          'metamask-color',
          'coinbase-color'
        ]}
        onClick={() => history.push(WalletRoutes.OnboardingImportTerms)}
      />
      <VerticalSpace space='8px' />
      <VerticalDivider />
      <VerticalSpace space='8px' />
      <ImportTypeAction
        title='Hardware wallet'
        description='Connect your hardware wallet with Brave'
        icons={['trezor-color', 'wallet-ledger']}
        onClick={() => history.push(WalletRoutes.OnboardingHardwareWalletTerms)}
      />
      <VerticalSpace space='165px' />
    </OnboardingContentLayout>
  )
}
