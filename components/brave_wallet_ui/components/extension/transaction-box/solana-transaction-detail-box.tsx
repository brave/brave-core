// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'

// types
import { BraveWallet, SerializableSolanaTxData } from '../../../constants/types'
import { TypedSolanaInstructionWithParams } from '../../../utils/solana-instruction-utils'

import {
  CodeSnippet,
  CodeSnippetText,
  DetailColumn,
  DetailText,
  TransactionText
} from './style'
import SolanaTransactionInstruction from '../../shared/solana-transaction-instruction/solana-transaction-instruction'

export interface Props {
  data: SerializableSolanaTxData | undefined
  instructions: TypedSolanaInstructionWithParams[] | undefined
  txType: BraveWallet.TransactionType
}

const txKeys = Object.keys(BraveWallet.TransactionType)

export const SolanaTransactionDetailBox = ({
  data,
  instructions,
  txType
}: Props) => {
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
      <DetailColumn>
        <TransactionText>{getLocale('braveWalletTransactionDetailBoxFunction')}:</TransactionText>
        <DetailText>{txKeys[txType]}</DetailText>
      </DetailColumn>

      <DetailColumn>
        {instructions?.map((instruction, index) => {
          return <SolanaTransactionInstruction
            key={index}
            typedInstructionWithParams={instruction}
          />
        })}
      </DetailColumn>
    </>
  )
}
