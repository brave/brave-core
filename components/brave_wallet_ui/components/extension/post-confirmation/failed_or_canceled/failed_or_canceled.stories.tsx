// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import {
  BraveWallet,
  StorybookCoinTypeOptions,
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
import { TransactionFailedOrCanceled } from './failed_or_canceled'

// Styled Components
import { LongWrapper } from '../../../../stories/style'
import { PanelWrapper } from '../../../../panel/style'

export const _TransactionFailedOrCanceled = {
  render: (args: StorybookTransactionArgs) => {
    // Props
    const { transactionType, coinType } = args

    // Computed
    const transaction = getPostConfirmationStatusMockTransaction(
      transactionType,
      BraveWallet.TransactionStatus.Error,
      coinType
    )

    return (
      <WalletPanelStory>
        <PanelWrapper
          width={390}
          height={650}
        >
          <LongWrapper padding='0px'>
            <TransactionFailedOrCanceled
              transaction={transaction}
              onClose={() => alert('Close panel screen clicked.')}
            />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default {
  component: TransactionFailedOrCanceled,
  argTypes: {
    transactionType: {
      options: StorybookTransactionOptions,
      control: { type: 'select' }
    },
    coinType: {
      options: StorybookCoinTypeOptions,
      control: { type: 'select' }
    }
  }
}
