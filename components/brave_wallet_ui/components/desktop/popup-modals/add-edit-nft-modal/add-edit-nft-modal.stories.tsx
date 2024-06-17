// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { AddOrEditNftModal } from './add-edit-nft-modal'

export const _AddNftModal = () => {
  return (
    <WalletPageStory>
      <AddOrEditNftModal
        onHideForm={() => alert('hide')}
        onClose={() => alert('close')}
      />
    </WalletPageStory>
  )
}

_AddNftModal.story = {
  name: 'Add NFT Modal'
}

export default _AddNftModal
