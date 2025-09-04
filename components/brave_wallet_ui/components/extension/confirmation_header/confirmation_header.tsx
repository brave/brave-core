// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  TransactionQueueSelector, //
} from '../transaction_queue_selector/transaction_queue_selector'

// Styled Components
import { HorizontalSpace, Row } from '../../shared/style'
import { HeaderText } from './confirmation_header.style'

interface Props {
  title: string
  transactionsQueueLength: number
  queueNextTransaction: () => void
  queuePreviousTransaction: () => void
  rejectAllTransactions: () => void
}

export function ConfirmationHeader(props: Props) {
  const {
    title,
    transactionsQueueLength,
    queueNextTransaction,
    queuePreviousTransaction,
    rejectAllTransactions,
  } = props

  return (
    <Row
      padding='18px'
      justifyContent={transactionsQueueLength > 1 ? 'space-between' : 'center'}
    >
      {transactionsQueueLength > 1 && <HorizontalSpace space='110px' />}
      <HeaderText textColor='primary'>{title}</HeaderText>
      <TransactionQueueSelector
        transactionsQueueLength={transactionsQueueLength}
        queueNextTransaction={queueNextTransaction}
        queuePreviousTransaction={queuePreviousTransaction}
        rejectAllTransactions={rejectAllTransactions}
      />
    </Row>
  )
}
