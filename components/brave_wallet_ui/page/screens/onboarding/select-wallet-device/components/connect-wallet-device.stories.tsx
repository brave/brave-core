// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.


import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { OnboardingSelectWalletDevice } from './onboarding-select-wallet-device'

export const _OnboardingSelectWalletDevice = () => {
  return (
    <WalletPageStory>
      <OnboardingSelectWalletDevice />
    </WalletPageStory>
  )
}

_OnboardingSelectWalletDevice.story = {
  name: 'OnboardingSelectWalletDevice'
}

export default {}
