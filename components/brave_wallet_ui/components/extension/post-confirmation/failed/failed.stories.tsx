// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '$web-common/locale'

// Components
import WalletPanelStory from '../../../../stories/wrappers/wallet-panel-story-wrapper'
import { StyledExtensionWrapper } from '../../../../stories/style'
import { PanelWrapper } from '../../../../panel/style'
import { TransactionFailed } from './failed'

export const _TransactionFailed = () => {
  const onClose = () => alert('Close panel screen')

  return (
    <WalletPanelStory>
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <TransactionFailed
            headerTitle='Swap 0.01 ETH to 32.2583 USDC'
            isPrimaryCTADisabled={false}
            errorDetailTitle={getLocale('braveWalletTransactionFailedModalSubtitle')}
            errorDetailContent='[ethjs-query] while formatting outputs from RPC ‘{“value”: {“code”:-32603,”data”: {“code”-32603,”message”:”Internal error”}}}’'
            onClose={onClose}
            onClickPrimaryCTA={() => alert('Clicked primary CTA')}
          />
        </StyledExtensionWrapper>
      </PanelWrapper>
    </WalletPanelStory>
  )
}

_TransactionFailed.story = {
  name: 'Transaction Failed'
}

export default _TransactionFailed
