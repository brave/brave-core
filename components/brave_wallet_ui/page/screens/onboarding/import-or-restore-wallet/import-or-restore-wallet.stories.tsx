// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import WalletPageStory from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { OnboardingImportOrRestoreWallet } from './import-or-restore-wallet'

export const OnboardingImportOrRestoreWalletStory = {
  render: () => {
    return (
      <WalletPageStory>
        <OnboardingImportOrRestoreWallet />
      </WalletPageStory>
    )
  }
}

export default {
  title: 'Import Or Restore Wallet',
  component: OnboardingImportOrRestoreWallet
}
