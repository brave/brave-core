// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import WalletPanelStory from '../../../../stories/wrappers/wallet-panel-story-wrapper'
import { StyledExtensionWrapper } from '../../../../stories/style'
import { PanelWrapper } from '../../../../panel/style'
import { TransactionConfirming } from './confirming'

export const _TransactionConfirming = () => {
  const onClose = () => alert('Close panel screen')

  return (
    <WalletPanelStory>
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <TransactionConfirming
            headerTitle='Swap 0.01 ETH to 32.2583 USDC'
            confirmations={8}
            confirmationsNeeded={12}
            onClose={onClose}
          />
        </StyledExtensionWrapper>
      </PanelWrapper>
    </WalletPanelStory>
  )
}

_TransactionConfirming.story = {
  name: 'Transaction Confirming'
}

export default _TransactionConfirming
