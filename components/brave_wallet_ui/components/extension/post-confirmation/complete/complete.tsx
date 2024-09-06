// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import LeoButton from '@brave/leo/react/button'
import {
  PluralStringProxyImpl //
} from 'chrome://resources/js/plural_string_proxy.js'
import usePromise from '$web-common/usePromise'

// Utils
import { getLocale } from '$web-common/locale'

// Hooks
import {
  usePendingTransactions //
} from '../../../../common/hooks/use-pending-transaction'

// Components
import {
  PostConfirmationHeader //
} from '../common/post_confirmation_header'

// Styled Components
import {
  // TransactionStatusDescription,
  Title,
  Wrapper,
  ErrorOrSuccessIconWrapper,
  ErrorOrSuccessIcon
} from '../common/common.style'
import { Column, Row, Text } from '../../../shared/style'
import { SerializableTransactionInfo } from '../../../../constants/types'
import { TransactionIntent } from '../common/transaction_intent'

interface Props {
  transaction: SerializableTransactionInfo
  onClose: () => void
  onClickViewInActivity: () => void
}

export const TransactionComplete = (props: Props) => {
  const { transaction, onClose, onClickViewInActivity } = props

  // Hooks
  const { transactionsQueueLength } = usePendingTransactions()

  // Computed
  const { result: pendingTransactionsLocale } = usePromise(
    async () =>
      PluralStringProxyImpl.getInstance().getPluralString(
        'braveWalletPendingTransactions',
        transactionsQueueLength
      ),
    [transactionsQueueLength]
  )
  const hasMoreTransactions = transactionsQueueLength >= 1

  return (
    <Wrapper
      fullWidth={true}
      fullHeight={true}
      alignItems='center'
      justifyContent='space-between'
    >
      <PostConfirmationHeader onClose={onClose} />
      <Column>
        <ErrorOrSuccessIconWrapper kind='success'>
          <ErrorOrSuccessIcon
            kind='success'
            name='check-normal'
          />
        </ErrorOrSuccessIconWrapper>
        <Title>{getLocale('braveWalletTransactionCompleteTitle')}</Title>
        <TransactionIntent transaction={transaction} />
      </Column>
      <Column
        fullWidth={true}
        padding='16px'
        gap='16px'
      >
        {hasMoreTransactions && (
          <Text
            textSize='12px'
            textColor='tertiary'
            isBold={false}
          >
            {pendingTransactionsLocale}
          </Text>
        )}
        <Row gap='8px'>
          <LeoButton
            kind={hasMoreTransactions ? 'outline' : 'filled'}
            onClick={onClickViewInActivity}
          >
            {getLocale('braveWalletViewInActivity')}
          </LeoButton>
          {hasMoreTransactions && (
            <LeoButton onClick={onClose}>
              {getLocale('braveWalletButtonNext')}
            </LeoButton>
          )}
        </Row>
      </Column>
    </Wrapper>
  )
}
