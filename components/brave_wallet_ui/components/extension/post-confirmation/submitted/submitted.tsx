// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useSelector } from 'react-redux'

// Constants
import { WalletState, BraveWallet } from '../../../../constants/types'

// Hooks
import { useExplorer } from '../../../../common/hooks'

// Utils
import { getLocale } from '$web-common/locale'

// Styled components
import { Panel } from '../..'
import { SubmittedIcon, Title } from './submitted.style'
import {
  ButtonRow,
  DetailButton,
  LinkIcon,
  TransactionStatusDescription
} from '../common/common.style'

interface Props {
  headerTitle: string
  transaction: BraveWallet.TransactionInfo
  onClose: () => void
}

export const TransactionSubmitted = (props: Props) => {
  const { headerTitle, transaction, onClose } = props

  // redux
  const selectedNetwork = useSelector(
    (state: { wallet: WalletState }) => state.wallet.selectedNetwork
  )
  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  return (
    <Panel navAction={onClose} title={headerTitle} headerStyle='slim'>
      <SubmittedIcon />
      <Title>{getLocale('braveWalletTransactionSubmittedTitle')}</Title>
      <TransactionStatusDescription>
        {getLocale('braveWalletTransactionSubmittedDescription')}
      </TransactionStatusDescription>
      <ButtonRow>
        <DetailButton onClick={onClickViewOnBlockExplorer('tx', transaction.txHash)}>
          {getLocale('braveWalletTransactionExplorer')}
        </DetailButton>
        <LinkIcon />
      </ButtonRow>
    </Panel>
  )
}
