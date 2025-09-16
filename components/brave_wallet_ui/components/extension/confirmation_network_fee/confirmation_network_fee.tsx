// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Queries
import {
  useGetDefaultFiatCurrencyQuery, //
} from '../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../constants/types'
import { ParsedTransaction } from '../../../utils/tx-utils'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

// Styled Components
import { Column, Row, VerticalDivider } from '../../shared/style'
import {
  ConfirmationInfoLabel,
  ConfirmationInfoText,
} from '../shared-panel-styles'

interface Props {
  gasFee: string
  transactionsNetwork?: BraveWallet.NetworkInfo
  transactionDetails?: ParsedTransaction
  onClickEditNetworkFee?: () => void
}

export function ConfirmationNetworkFee(props: Props) {
  const {
    transactionsNetwork,
    gasFee,
    transactionDetails,
    onClickEditNetworkFee,
  } = props

  // Queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  return (
    <>
      <Row justifyContent='space-between'>
        <ConfirmationInfoLabel textColor='secondary'>
          {getLocale('braveWalletTransactionDetailNetwork')}
        </ConfirmationInfoLabel>
        <ConfirmationInfoLabel textColor='primary'>
          {transactionsNetwork?.chainName ?? ''}
        </ConfirmationInfoLabel>
      </Row>
      <VerticalDivider />
      <Row justifyContent='space-between'>
        <Column
          alignItems='flex-start'
          justifyContent='flex-start'
          gap='4px'
        >
          <ConfirmationInfoLabel
            textColor='secondary'
            textAlign='left'
          >
            {getLocale('braveWalletAllowSpendTransactionFee')}
          </ConfirmationInfoLabel>
          {onClickEditNetworkFee && (
            <Button
              size='tiny'
              kind='plain'
              onClick={onClickEditNetworkFee}
            >
              <Icon
                name='tune'
                slot='icon-before'
              />
              {getLocale('braveWalletAllowSpendEditButton')}
            </Button>
          )}
        </Column>
        <Column
          alignItems='flex-end'
          justifyContent='flex-start'
        >
          <ConfirmationInfoLabel
            textColor='primary'
            textAlign='right'
          >
            {(transactionsNetwork
              && new Amount(gasFee)
                .divideByDecimals(transactionsNetwork.decimals)
                .formatAsAsset(6, transactionsNetwork.symbol))
              || ''}
          </ConfirmationInfoLabel>
          <ConfirmationInfoText
            textColor='tertiary'
            textAlign='right'
          >
            {new Amount(transactionDetails?.gasFeeFiat ?? '').formatAsFiat(
              defaultFiatCurrency,
            )}
          </ConfirmationInfoText>
        </Column>
      </Row>
    </>
  )
}
