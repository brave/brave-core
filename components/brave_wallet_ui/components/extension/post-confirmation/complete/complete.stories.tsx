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
import { TransactionComplete } from './complete'

export const _TransactionComplete = () => {
  const onClose = () => alert('Close panel screen')

  return (
    <WalletPanelStory>
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <TransactionComplete
            headerTitle='Swap 0.01 ETH to 32.2583 USDC'
            description={getLocale('braveWalletTransactionCompleteDescription')}
            isPrimaryCTADisabled={false}
            primaryCTAText={getLocale('braveWalletButtonNext')}
            onClose={onClose}
            onClickPrimaryCTA={() => alert('Clicked primary CTA')}
            onClickSecondaryCTA={() => alert('Clicked secondary CTA')}
          />
        </StyledExtensionWrapper>
      </PanelWrapper>
    </WalletPanelStory>
  )
}

_TransactionComplete.story = {
  name: 'Transaction Complete'
}

export default _TransactionComplete
