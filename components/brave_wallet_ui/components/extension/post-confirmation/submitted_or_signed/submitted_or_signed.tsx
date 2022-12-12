// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useSelector } from 'react-redux'

// Constants
import { BraveWallet, WalletState, SerializableTransactionInfo } from '../../../../constants/types'

// Hooks
import { useExplorer } from '../../../../common/hooks'

// Utils
import { getLocale } from '$web-common/locale'

// Styled components
import { Panel } from '../..'
import { SubmittedOrSignedIcon, Title } from './submitted_or_signed.style'
import {
  ButtonRow,
  DetailButton,
  LinkIcon,
  TransactionStatusDescription
} from '../common/common.style'

interface Props {
  headerTitle: string
  transaction: SerializableTransactionInfo
  onClose: () => void
}

export const TransactionSubmittedOrSigned = (props: Props) => {
  const { headerTitle, transaction, onClose } = props

  // redux
  const selectedNetwork = useSelector(
    (state: { wallet: WalletState }) => state.wallet.selectedNetwork
  )
  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  const title =
    transaction.txStatus === BraveWallet.TransactionStatus.Submitted
      ? getLocale('braveWalletTransactionSubmittedTitle')
      : getLocale('braveWalletTransactionSignedTitle')
  const description =
    transaction.txStatus === BraveWallet.TransactionStatus.Submitted
     ? getLocale('braveWalletTransactionSubmittedDescription')
     : getLocale('braveWalletTransactionSignedDescription')

  return (
    <Panel navAction={onClose} title={headerTitle} headerStyle='slim'>
      <SubmittedOrSignedIcon />
      <Title>{title}</Title>
      <TransactionStatusDescription>
        {description}
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
