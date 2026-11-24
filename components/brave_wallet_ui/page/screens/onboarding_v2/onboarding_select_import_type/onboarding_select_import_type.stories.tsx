// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  WalletPageStory, //
} from '$wallet/stories/wrappers/wallet-page-story-wrapper'
import { OnboardingSelectImportType } from './onboarding_select_import_type'

export const _OnboardingSelectImportType = {
  title: 'Onboarding Select Import Type',
  render: () => {
    return (
      <WalletPageStory>
        <OnboardingSelectImportType />
      </WalletPageStory>
    )
  },
}

export default {
  title: 'Wallet/Desktop/Screens/OnboardingV2',
  component: OnboardingSelectImportType,
}
