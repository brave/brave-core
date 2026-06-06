// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { formatLocale, getLocale } from '../../../../common/locale'

// Styled Components
import {
  Header,
  Title,
  Description,
  Backdrop,
  Warning,
  Button,
} from './approval_for_all_warning.style'
import { Column, Row } from '../../shared/style'

interface Props {
  onConfirm: () => void
  onReject: () => void
  address: string
}

export function ApprovalForAllWarning(props: Props) {
  const { onConfirm, onReject, address } = props

  const operators = address.split(',').filter(Boolean)

  const description = React.useMemo(() => {
    if (operators.length === 1) {
      return formatLocale('braveWalletApprovalForAllFinalWarningDescription', {
        $1: <b>{reduceAddress(operators[0])}</b>,
      })
    }
    return (
      <>
        {getLocale('braveWalletApprovalForAllFinalWarningDescriptionMultiple')}
        <ul>
          {operators.map((op) => (
            <li key={op}>
              <b>{reduceAddress(op)}</b>
            </li>
          ))}
        </ul>
      </>
    )
  }, [operators])

  return (
    <Backdrop>
      <Warning
        padding='24px'
        justifyContent='space-between'
        width='100%'
      >
        <Row justifyContent='flex-start'>
          <Header textColor='primary'>{getLocale('braveWalletWarning')}</Header>
        </Row>
        <Column
          width='100%'
          padding='72px 0px'
          gap='10px'
        >
          <Icon name='warning-circle-filled' />
          <Title textColor='primary'>
            {getLocale('braveWalletApprovalForAllFinalWarningTitle')}
          </Title>
          <Description textColor='primary'>{description}</Description>
        </Column>
        <Column
          width='100%'
          gap='16px'
        >
          <Button onClick={onReject}>
            {getLocale('braveWalletTransactionCancel')}
          </Button>
          <Button
            onClick={onConfirm}
            kind='plain-faint'
          >
            {getLocale('braveWalletConfirmAnyway')}
          </Button>
        </Column>
      </Warning>
    </Backdrop>
  )
}
