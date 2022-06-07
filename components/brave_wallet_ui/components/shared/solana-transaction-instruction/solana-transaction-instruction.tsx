// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import Amount from '../../../utils/amount'
import { getSolanaProgramIdName } from '../../../utils/solana-program-id-utils'

// types
import { SolanaInstructionParamKeys, TypedSolanaInstructionWithParams } from '../../../utils/solana-instruction-utils'

// styles
import { DetailColumn } from '../../desktop/portfolio-transaction-item/style'
import {
  Divider,
  SectionRow,
  TransactionTitle,
  TransactionTypeText
} from '../../extension/confirm-transaction-panel/style'
import { InstructionParamBox } from './solana-transaction-instruction.style'
import { LAMPORTS_PER_SOL } from '@solana/web3.js'

interface Props {
  typedInstructionWithParams: TypedSolanaInstructionWithParams
}

const getSolanaParamKeyName = (key: SolanaInstructionParamKeys) => {
  return ({
    fromPubkey: 'From Address: ',
    toPubkey: 'To Address: ',
    lamports: 'Transfer Amount: '
  } as Partial<Record<SolanaInstructionParamKeys, string>>)[key] || key
}

export const SolanaTransactionInstruction: React.FC<Props> = ({
  typedInstructionWithParams: {
    instruction: {
      programId,
      data
    },
    type,
    params
  }
}) => {
  return <>
    <SectionRow>
      <TransactionTitle>
        {getSolanaProgramIdName(programId)} - {type}
      </TransactionTitle>
    </SectionRow>

    {Object.keys(params).length > 0 && (
      <>
        <Divider />

        <DetailColumn>
          <TransactionTypeText>
            {
              Object.entries(params).map(([key, value]) => {
                return <InstructionParamBox key={key}>
                  <var>{getSolanaParamKeyName(
                    key as SolanaInstructionParamKeys
                  )}: </var>
                  <samp>{
                    (key as SolanaInstructionParamKeys === 'lamports'
                      ? new Amount(value.toString())
                      .div(LAMPORTS_PER_SOL)
                      .formatAsAsset(9, 'SOL')
                      : value
                    ).toString()
                  }</samp>
                </InstructionParamBox>
              })
            }
          </TransactionTypeText>
        </DetailColumn>
      </>
    )}
  </>
}

export default SolanaTransactionInstruction
