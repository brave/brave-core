// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mocks
import { mockTransactionInfo } from '../../../../stories/mock-data/mock-transaction-info'

// Components
import WalletPanelStory from '../../../../stories/wrappers/wallet-panel-story-wrapper'
import { StyledExtensionWrapper } from '../../../../stories/style'
import { PanelWrapper } from '../../../../panel/style'
import { TransactionSubmittedOrSigned } from './submitted_or_signed'

export const _TransactionSubmitted = () => {
  const onClose = () => alert('Close panel screen')

  return (
    <WalletPanelStory>
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <TransactionSubmittedOrSigned
            onClose={onClose}
            transaction={mockTransactionInfo}
            headerTitle='Swap 0.01 ETH to 32.2583 USDC'
          />
        </StyledExtensionWrapper>
      </PanelWrapper>
    </WalletPanelStory>
  )
}

_TransactionSubmitted.story = {
  name: 'Transaction Submitted/Signed'
}

export default _TransactionSubmitted
