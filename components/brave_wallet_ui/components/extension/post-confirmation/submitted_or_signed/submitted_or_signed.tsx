// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Constants
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../../../constants/types'

// Hooks
import { useExplorer } from '../../../../common/hooks/explorer'
import { useTransactionsNetwork } from '../../../../common/hooks/use-transactions-network'

// Utils
import { getLocale } from '$web-common/locale'

// Styled components
import { Panel } from '../../panel/index'
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

  // custom hooks
  const transactionNetwork = useTransactionsNetwork(transaction)
  const onClickViewOnBlockExplorer = useExplorer(transactionNetwork)

  const title =
    transaction.txStatus === BraveWallet.TransactionStatus.Submitted
      ? getLocale('braveWalletTransactionSubmittedTitle')
      : getLocale('braveWalletTransactionSignedTitle')
  const description =
    transaction.txStatus === BraveWallet.TransactionStatus.Submitted
      ? getLocale('braveWalletTransactionSubmittedDescription')
      : getLocale('braveWalletTransactionSignedDescription')

  return (
    <Panel
      navAction={onClose}
      title={headerTitle}
      headerStyle='slim'
    >
      <SubmittedOrSignedIcon />
      <Title>{title}</Title>
      <TransactionStatusDescription>{description}</TransactionStatusDescription>
      <ButtonRow>
        <DetailButton
          onClick={onClickViewOnBlockExplorer(
            transaction.swapInfo?.provider === 'lifi' ? 'lifi' : 'tx',
            transaction.txHash
          )}
        >
          {getLocale('braveWalletTransactionExplorer')}
        </DetailButton>
        <LinkIcon />
      </ButtonRow>
    </Panel>
  )
}
