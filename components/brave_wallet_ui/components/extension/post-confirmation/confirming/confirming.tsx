// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useSelector } from 'react-redux'

// Constants
import { WalletState } from '../../../../constants/types'

// Hooks
import { useExplorer } from '../../../../common/hooks'

// Utils
import { getLocale } from '$web-common/locale'

// Styled components
import { Panel } from '../..'
import { ConfirmationsNumber, ConfirmingIcon, ConfirmingText, Title } from './confirming.style'
import {
  ButtonRow,
  DetailButton,
  LinkIcon,
  TransactionStatusDescription
} from '../common/common.style'

interface Props {
  headerTitle: string
  confirmations: number
  confirmationsNeeded: number
  onClose: () => void
}

export const TransactionConfirming = (props: Props) => {
  const { headerTitle, confirmations, confirmationsNeeded, onClose } = props

  // redux
  const selectedNetwork = useSelector(
    (state: { wallet: WalletState }) => state.wallet.selectedNetwork
  )
  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  return (
    <Panel navAction={onClose} title={headerTitle} headerStyle='slim'>
      <ConfirmingIcon>
        <ConfirmingText>{getLocale('braveWalletTransactionConfirmingText')}</ConfirmingText>
        <ConfirmationsNumber>
          {confirmations} / {confirmationsNeeded}
        </ConfirmationsNumber>
      </ConfirmingIcon>

      <Title>{getLocale('braveWalletTransactionConfirmingTitle')}</Title>

      <TransactionStatusDescription>
        {getLocale('braveWalletTransactionConfirmingDescription')}
      </TransactionStatusDescription>
      <ButtonRow>
        <DetailButton onClick={onClickViewOnBlockExplorer('tx', '0x1')}>
          {getLocale('braveWalletTransactionExplorer')}
        </DetailButton>
        <LinkIcon />
      </ButtonRow>
    </Panel>
  )
}
