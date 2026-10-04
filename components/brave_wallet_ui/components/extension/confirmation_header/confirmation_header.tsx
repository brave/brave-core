// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Components
import {
  TransactionQueueSelector, //
} from '../transaction_queue_selector/transaction_queue_selector'

// Styled Components
import { HorizontalSpace, Row, Text } from '../../shared/style'
import { HeaderText, CloseButton } from './confirmation_header.style'

interface Props {
  title: string
  transactionsQueueLength: number
  queueNextTransaction: () => void
  queuePreviousTransaction: () => void
  rejectAllTransactions: () => void
  close: () => void
}

export function ConfirmationHeader(props: Props) {
  const {
    title,
    transactionsQueueLength,
    queueNextTransaction,
    queuePreviousTransaction,
    rejectAllTransactions,
    close,
  } = props

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isSidePanel = useSafeUISelector(UISelectors.isSidePanel)
  const isOnlyPanel = isPanel && !isSidePanel

  // Only used on the Panel (not Side Panel).
  if (isOnlyPanel) {
    return (
      <Row
        padding='18px'
        justifyContent={
          transactionsQueueLength > 1 ? 'space-between' : 'center'
        }
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

  return (
    <Row
      padding='32px'
      justifyContent='space-between'
    >
      <Text
        textColor='primary'
        variant='heading.h2'
      >
        {title}
      </Text>
      <CloseButton
        fab
        size='large'
        kind='plain-faint'
        onClick={close}
      >
        <Icon name='close' />
      </CloseButton>
    </Row>
  )
}
