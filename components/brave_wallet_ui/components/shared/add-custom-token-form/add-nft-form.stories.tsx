// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import WalletPageStory from '../../../stories/wrappers/wallet-page-story-wrapper'
import { AddNftForm } from './add-nft-form'

export const _AddNftForm = () => {
  const [contractAddress, setContractAddress] = React.useState('')
  return (
    <WalletPageStory>
      <AddNftForm
        contractAddress={contractAddress}
        onHideForm={() => undefined}
        onTokenFound={setContractAddress}
        onChangeContractAddress={setContractAddress}
      />
    </WalletPageStory>
  )
}

_AddNftForm.story = {
  name: 'Add NFT Form'
}

export default _AddNftForm
