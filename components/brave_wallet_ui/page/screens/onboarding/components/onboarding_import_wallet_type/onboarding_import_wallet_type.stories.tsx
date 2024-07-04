// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import {
  WalletPageStory //
} from '../../../../../stories/wrappers/wallet-page-story-wrapper'
import { OnboardingImportWalletType } from './import_wallet_type'

export const _OnboardingImportWalletType = {
  title: 'Select Import Wallet Type',
  render: () => {
    return (
      <WalletPageStory>
        <OnboardingImportWalletType />
      </WalletPageStory>
    )
  }
}

export default { component: OnboardingImportWalletType }
