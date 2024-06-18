// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { EthSignTypedData } from './eth_sign_typed_data'

// mocks
import {
  mockSignMessageRequest //
} from '../../../../stories/mock-data/mock-eth-requests'

export const _EthSignTypedData = {
  render: () => {
    return (
      <EthSignTypedData
        data={mockSignMessageRequest.signData.ethSignTypedData}
        width={'100%'}
        height={'100%'}
      />
    )
  }
}

export default { component: EthSignTypedData }
