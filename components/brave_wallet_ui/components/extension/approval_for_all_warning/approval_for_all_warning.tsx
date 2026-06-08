// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { formatLocale, getLocale } from '../../../../common/locale'

// Components
import { Tooltip } from '../../shared/tooltip/index'
import {
  ChainInfo,
  InlineViewOnBlockExplorerIconButton,
} from '../confirm-transaction-panel/common/view_on_explorer_button'

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
  network?: ChainInfo | null
}

export function ApprovalForAllWarning(props: Props) {
  const { onConfirm, onReject, address, network } = props

  const operators = address.split(',').filter(Boolean)

  const description = React.useMemo(() => {
    const operatorRow = (op: string) => (
      <Row
        justifyContent='flex-start'
        gap='4px'
      >
        <Tooltip
          text={op}
          isAddress
          verticalPosition='above'
        >
          <b>{reduceAddress(op)}</b>
        </Tooltip>
        {network && (
          <InlineViewOnBlockExplorerIconButton
            address={op}
            network={network}
            urlType='address'
          />
        )}
      </Row>
    )

    if (operators.length === 1) {
      return formatLocale('braveWalletApprovalForAllFinalWarningDescription', {
        $1: operatorRow(operators[0]),
      })
    }
    return (
      <>
        {getLocale('braveWalletApprovalForAllFinalWarningDescriptionMultiple')}
        <ul>
          {operators.map((op) => (
            <li key={op}>{operatorRow(op)}</li>
          ))}
        </ul>
      </>
    )
  }, [operators, network])

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
            kind='plain'
          >
            {getLocale('braveWalletConfirmAnyway')}
          </Button>
        </Column>
      </Warning>
    </Backdrop>
  )
}
