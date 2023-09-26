// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import WalletPageStory from '../../../../../../stories/wrappers/wallet-page-story-wrapper'
import { DappDetailsModal } from './dapp-details-modal'
import { mockDapp } from '../../../../../../common/constants/mocks'

export const _DappDetailsModal = () => {
  return (
    <WalletPageStory>
      <DappDetailsModal onClose={() => {}} dapp={mockDapp} />
    </WalletPageStory>
  )
}

export default _DappDetailsModal.storyName = 'Dapp Details Modal'