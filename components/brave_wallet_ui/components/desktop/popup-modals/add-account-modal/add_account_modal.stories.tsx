// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { AddAccountModal } from './add-account-modal'

export const _AddAccountModal = () => {
  return (
    <WalletPageStory>
      <AddAccountModal />
    </WalletPageStory>
  )
}

_AddAccountModal.storyName = 'Add Account Modal'

export default _AddAccountModal
