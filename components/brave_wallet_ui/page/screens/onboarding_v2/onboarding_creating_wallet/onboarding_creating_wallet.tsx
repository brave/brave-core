// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '$web-common/locale'

// Components
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/onboarding_content_layout'

// Styled Components
import { LoadingIcon } from './onboarding_creating_wallet.style'
import { Column, Text } from '$wallet/components/shared/style'

export const OnboardingCreatingWallet = () => {
  return (
    <OnboardingContentLayout showBackButton={false}>
      <Column
        fullWidth={true}
        height='256px'
        alignItems='center'
        justifyContent='center'
        gap='20px'
      >
        <LoadingIcon />
        <Text
          variant='heading.h2'
          textColor='primary'
        >
          {getLocale(S.BRAVE_WALLET_CREATING_WALLET)}
        </Text>
      </Column>
    </OnboardingContentLayout>
  )
}
