// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'

import { BraveWallet } from '../../../constants/types'
import {
  CodeSnippet,
  CodeSnippetText,
  DetailColumn,
  DetailRow,
  DetailText,
  TransactionText
} from './style'
import { ParsedTransaction } from '../../../common/hooks/transaction-parser'
import SolanaTransactionInstruction from '../../shared/solana-transaction-instruction/solana-transaction-instruction'

export interface Props {
  transactionInfo: BraveWallet.TransactionInfo
  transactionDetails: ParsedTransaction
}

const txKeys = Object.keys(BraveWallet.TransactionType)

export const SolanaTransactionDetailBox = ({
  transactionDetails,
  transactionInfo
}: Props) => {
  const {
    txArgs,
    txParams,
    txType
  } = transactionInfo
  const data = transactionInfo.txDataUnion?.solanaTxData
  const { instructions } = transactionDetails

  if (!data) {
    return (
      <CodeSnippet>
        <code>
          <CodeSnippetText>{getLocale('braveWalletConfirmTransactionNoData')}</CodeSnippetText>
        </code>
      </CodeSnippet>
    )
  }

  return (
    <>
      <DetailRow>
        <TransactionText>{getLocale('braveWalletTransactionDetailBoxFunction')}:</TransactionText>
        <DetailText>{txKeys[txType]}</DetailText>
      </DetailRow>

      <DetailColumn>
        {instructions?.map((instruction, index) => {
          return <SolanaTransactionInstruction
            key={index}
            typedInstructionWithParams={instruction}
          />
        })}
      </DetailColumn>

      {txParams.map((param, i) =>
        <CodeSnippetText
          key={i}
          as='code'
        >
          {param}:{' '}{txArgs[i] || ''}
        </CodeSnippetText>
      )}
    </>
  )
}
