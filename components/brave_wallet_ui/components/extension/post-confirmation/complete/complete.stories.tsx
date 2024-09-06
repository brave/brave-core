// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import {
  BraveWallet,
  StorybookTransactionArgs,
  StorybookTransactionOptions
} from '../../../../constants/types'

// Mocks
import {
  getPostConfirmationStatusMockTransaction //
} from '../../../../stories/mock-data/mock-transaction-info'

// Components
import {
  WalletPanelStory //
} from '../../../../stories/wrappers/wallet-panel-story-wrapper'
import { TransactionComplete } from './complete'

// Styled Components
import { LongWrapper } from '../../../../stories/style'
import { PanelWrapper } from '../../../../panel/style'

export const _TransactionComplete = {
  render: (args: StorybookTransactionArgs) => {
    // Props
    const { transactionType } = args

    // Computed
    const transaction = getPostConfirmationStatusMockTransaction(
      transactionType,
      BraveWallet.TransactionStatus.Confirmed
    )

    return (
      <WalletPanelStory>
        <PanelWrapper
          width={390}
          height={650}
        >
          <LongWrapper padding='0px'>
            <TransactionComplete
              transaction={transaction}
              onClose={() => alert('Close panel screen clicked.')}
              onClickViewInActivity={() => alert('View in activity clicked.')}
            />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default {
  component: TransactionComplete,
  argTypes: {
    transactionType: {
      options: StorybookTransactionOptions,
      control: { type: 'select' }
    }
  }
}
