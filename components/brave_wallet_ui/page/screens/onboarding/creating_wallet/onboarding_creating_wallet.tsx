// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'

// components
import {
  OnboardingContentLayout //
} from '../components/onboarding_content_layout/content_layout'

// Style
import {
  CreatingWalletText,
  LoadingIcon
} from './onboarding_creating_wallet.style'
import { Column } from '../../../../components/shared/style'

export const OnboardingCreatingWallet = () => {
  return (
    <OnboardingContentLayout
      showBackButton={false}
      centerContent={true}
    >
      <Column
        fullWidth={true}
        alignItems='center'
        justifyContent='center'
      >
        <LoadingIcon />
        <CreatingWalletText>
          {getLocale('braveWalletCreatingWallet')}
        </CreatingWalletText>
      </Column>
    </OnboardingContentLayout>
  )
}
