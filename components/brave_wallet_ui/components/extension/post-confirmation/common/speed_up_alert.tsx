// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Button from '@brave/leo/react/button'
import { useDispatch } from 'react-redux'

// Actions
import * as WalletPanelActions from '../../../../panel/actions/wallet_panel_actions'

// Utils
import { getLocale } from '$web-common/locale'
import { getCoinFromTxDataUnion } from '../../../../utils/network-utils'

// Types
import { SerializableTransactionInfo } from '../../../../constants/types'

// Hooks
import {
  useSpeedupTransactionMutation //
} from '../../../../common/slices/api.slice'

// Styled Components
import { Alert } from './common.style'

interface Props {
  transaction: SerializableTransactionInfo
}

export const SpeedUpAlert = (props: Props) => {
  const { transaction } = props

  // Computed
  const txCoinType = getCoinFromTxDataUnion(transaction.txDataUnion)

  // Hooks
  const [speedupTx] = useSpeedupTransactionMutation()
  const dispatch = useDispatch()

  // Methods
  const onClickSpeedUpTransaction = () => {
    speedupTx({
      coinType: txCoinType,
      chainId: transaction.chainId,
      transactionId: transaction.id
    })
    dispatch(WalletPanelActions.setSelectedTransactionId(undefined))
  }

  return (
    <Alert type='info'>
      {getLocale('braveWalletTransactionTakingLongTime')}
      <div slot='content-after'>
        <Button
          onClick={onClickSpeedUpTransaction}
          kind='outline'
          size='tiny'
        >
          {getLocale('braveWalletTransactionSpeedup')}
        </Button>
      </div>
    </Alert>
  )
}
