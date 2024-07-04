// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import OnboardingCreatePassword from './onboarding_create_password'

export const _OnboardingCreatePassword = {
  title: 'Create Password',
  render: () => {
    return (
      <WalletPageStory>
        <OnboardingCreatePassword onWalletCreated={() => { }} />
      </WalletPageStory>
    )
  }
}

export default { component: OnboardingCreatePassword }
