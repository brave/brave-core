// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { Origin, TransactionOrigin } from './origin'

// styles
import { Column } from '../../../shared/style'

// mocks
import {
  mockBraveWalletOrigin,
  mockOriginInfo,
  mockUniswapOriginInfo
} from '../../../../stories/mock-data/mock-origin-info'
import { mockERC20Token } from '../../../../stories/mock-data/mock-asset-options'
import { mockEthMainnet } from '../../../../stories/mock-data/mock-networks'

export const _LongOrigin = {
  render: () => {
    return (
      <Column>
        <Origin originInfo={mockOriginInfo} />
      </Column>
    )
  }
}

export const _Origin = {
  render: () => {
    return (
      <Column>
        <Origin originInfo={mockUniswapOriginInfo} />
      </Column>
    )
  }
}

export const BraveOrigin = {
  render: () => {
    return (
      <Column>
        <Origin originInfo={mockBraveWalletOrigin} />
      </Column>
    )
  }
}

export const _TransactionOrigin = {
  render: () => {
    return (
      <Column>
        <TransactionOrigin
          network={mockEthMainnet}
          originInfo={mockUniswapOriginInfo}
          contractAddress={mockERC20Token.contractAddress}
        />
      </Column>
    )
  }
}

export default { title: 'Origin Info' }
