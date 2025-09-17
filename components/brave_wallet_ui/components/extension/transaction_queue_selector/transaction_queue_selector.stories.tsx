// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  TransactionQueueSelector, //
} from './transaction_queue_selector'

// Shared Styles
import { Column } from '../../shared/style'

export const _TransactionQueueSelector = {
  render: () => {
    return (
      <TransactionQueueSelector
        transactionsQueueLength={4}
        queueNextTransaction={() => alert('Queue Next Transaction')}
        queuePreviousTransaction={() => alert('Queue Previous Transaction')}
        rejectAllTransactions={() => alert('Reject All Transactions')}
      />
    )
  },
}

export default {
  title: 'Wallet/Panel/Components/Transaction Queue Selector',
  component: TransactionQueueSelector,
  parameters: {
    layout: 'centered',
  },
  decorators: [
    (Story: any) => (
      <Column
        width='400px'
        height='400px'
        alignItems='center'
        justifyContent='center'
      >
        <Story />
      </Column>
    ),
  ],
}
