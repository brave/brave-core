// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { getLocale } from '$web-common/locale'

import { NavButton, Panel } from '../..'
import {
  SuccessIcon,
  Title
} from './complete.style'
import {
  ButtonRow,
  TransactionStatusDescription
} from '../common/common.style'

interface Props {
  headerTitle: string
  description: string
  isPrimaryCTADisabled: boolean
  onClose: () => void
  onClickSecondaryCTA: () => void
  onClickPrimaryCTA: () => void
}

export const TransactionComplete = (props: Props) => {
  const {
    headerTitle,
    description,
    isPrimaryCTADisabled,
    onClose,
    onClickPrimaryCTA,
    onClickSecondaryCTA
  } = props

  return (
    <Panel
      navAction={onClose}
      title={headerTitle}
      headerStyle='slim'
    >
      <SuccessIcon />
      <Title>{getLocale('braveWalletTransactionCompleteTitle')}</Title>
      <TransactionStatusDescription>{description}</TransactionStatusDescription>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletTransactionCompleteReceiptCTA')}
          onSubmit={onClickSecondaryCTA}
        />
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletButtonDone')}
          onSubmit={onClickPrimaryCTA}
          disabled={isPrimaryCTADisabled}
        />
      </ButtonRow>
    </Panel>
  )
}
