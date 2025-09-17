// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled components
import { Button, ButtonMenu } from './transaction_queue_selector.style'
import { Row } from '../../shared/style'

interface Props {
  transactionsQueueLength: number
  queueNextTransaction: () => void
  queuePreviousTransaction: () => void
  rejectAllTransactions: () => void
}

export function TransactionQueueSelector(props: Props) {
  const {
    transactionsQueueLength,
    queueNextTransaction,
    queuePreviousTransaction,
    rejectAllTransactions,
  } = props
  const [showMenu, setShowMenu] = React.useState(false)

  if (transactionsQueueLength <= 1) {
    return null
  }

  return (
    <ButtonMenu
      isOpen={showMenu}
      onChange={({ isOpen }) => setShowMenu(isOpen)}
      onClose={() => setShowMenu(false)}
      placement='bottom-end'
    >
      <Button
        size='tiny'
        kind='plain'
        slot='anchor-content'
      >
        {getLocale('braveWalletPendingTransactionsNumber').replace(
          '$1',
          transactionsQueueLength.toString(),
        )}
        <Icon
          slot='icon-after'
          name='more-vertical'
        />
      </Button>
      <leo-menu-item onClick={queueNextTransaction}>
        <Icon name='carat-right' />
        {getLocale('braveWalletNextTransaction')}
      </leo-menu-item>
      <leo-menu-item onClick={queuePreviousTransaction}>
        <Row
          justifyContent='flex-start'
          gap='8px'
        >
          <Icon name='carat-left' />
          {getLocale('braveWalletPreviousTransaction')}
        </Row>
      </leo-menu-item>
      <leo-menu-item onClick={rejectAllTransactions}>
        <Row
          justifyContent='flex-start'
          gap='8px'
        >
          <Icon name='close-circle' />
          {getLocale('braveWalletRejectTransactions').replace(
            '$1',
            transactionsQueueLength.toString(),
          )}
        </Row>
      </leo-menu-item>
    </ButtonMenu>
  )
}

export default TransactionQueueSelector
