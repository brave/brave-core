// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { SerializableTransactionInfo } from '../../../../constants/types'

// Utils
import { getLocale } from '$web-common/locale'
import { getCoinFromTxDataUnion } from '../../../../utils/network-utils'

// Hooks
import {
  useCancelTransactionMutation //
} from '../../../../common/slices/api.slice'

// Components
import { PostConfirmationHeader } from '../common/post_confirmation_header'

// Styled components
import { CancelButton } from './cancel_transaction.style'
import { Wrapper, Title } from '../common/common.style'
import { Column, Row, Text } from '../../../shared/style'

interface Props {
  transaction: SerializableTransactionInfo
  onBack: () => void
}

export const CancelTransaction = (props: Props) => {
  const { transaction, onBack } = props

  // Computed
  const txCoinType = getCoinFromTxDataUnion(transaction.txDataUnion)

  // Hooks
  const [cancelTx] = useCancelTransactionMutation()

  // Methods
  const onClickCancelTransaction = () => {
    cancelTx({
      coinType: txCoinType,
      chainId: transaction.chainId,
      transactionId: transaction.id
    })
  }

  return (
    <Wrapper
      fullWidth={true}
      fullHeight={true}
      alignItems='center'
      justifyContent='space-between'
    >
      <PostConfirmationHeader onBack={onBack} />
      <Column
        fullWidth={true}
        padding='0px 24px'
        gap='8px'
      >
        <Title>{getLocale('braveWalletTransactionCancel')}</Title>
        <Text
          textSize='14px'
          textColor='primary'
          isBold={false}
        >
          {getLocale('braveWalletCancelTransactionDescription')}
        </Text>
      </Column>
      <Row padding='16px'>
        <CancelButton onClick={onClickCancelTransaction}>
          {getLocale('braveWalletTransactionCancel')}
        </CancelButton>
      </Row>
    </Wrapper>
  )
}
