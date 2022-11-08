// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { LAMPORTS_PER_SOL } from '@solana/web3.js'

// utils
import Amount from '../../../utils/amount'
import { getSolanaProgramIdName } from '../../../utils/solana-program-id-utils'
import { findAccountName } from '../../../utils/account-utils'
import { getLocale } from '../../../../common/locale'

// types
import { WalletState } from '../../../constants/types'
import { getSolanaInstructionParamKeyName, SolanaInstructionParamKeys, TypedSolanaInstructionWithParams } from '../../../utils/solana-instruction-utils'

// styles
import {
  Divider,
  SectionRow,
  TransactionTitle
} from '../../extension/confirm-transaction-panel/style'

import {
  InstructionBox,
  InstructionParamBox,
  AddressText,
  CodeSectionTitle
} from './solana-transaction-instruction.style'

import {
  CodeSnippet,
  CodeSnippetText
} from '../../extension/transaction-box/style'

import Tooltip from '../tooltip'

interface Props {
  typedInstructionWithParams: TypedSolanaInstructionWithParams
}

export const SolanaTransactionInstruction: React.FC<Props> = ({
  typedInstructionWithParams: {
    instruction: {
      programId,
      data,
      keys
    },
    type,
    params
  }
}) => {
  // redux
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)

  // render
  return (
    <InstructionBox>
      {type === 'Unknown' ? (
        <>
          {keys.length > 0 &&
            <>
              <CodeSectionTitle>{getLocale('braveWalletSolanaAccounts')}</CodeSectionTitle>
              {keys.map((entry, i) => {
                return (
                  <CodeSnippet
                    key={i}
                  >
                    <code>
                      <CodeSnippetText>{entry.pubkey.toString()}</CodeSnippetText>
                    </code>
                  </CodeSnippet>
                )
              })}
            </>
          }
          {data &&
            <>
              <CodeSectionTitle>{getLocale('braveWalletSolanaData')}</CodeSectionTitle>
              <CodeSnippet>
                <code>
                  <CodeSnippetText>{JSON.stringify(data)}</CodeSnippetText>
                </code>
              </CodeSnippet>
            </>
          }
          {programId &&
            <>
              <CodeSectionTitle>{getLocale('braveWalletSolanaProgramID')}</CodeSectionTitle>
              <CodeSnippet>
                <code>
                  <CodeSnippetText>{JSON.stringify(programId)}</CodeSnippetText>
                </code>
              </CodeSnippet>
            </>
          }
        </>
      ) : (
        <>
          <SectionRow>
            <TransactionTitle>
              {getSolanaProgramIdName(programId)} - {type}
            </TransactionTitle>
          </SectionRow>
          {Object.keys(params).length > 0 && (
            <>
              <Divider />
              {
                Object.entries(params).map(([key, value]) => {
                  const paramName = getSolanaInstructionParamKeyName(key as SolanaInstructionParamKeys)
                  const isAddressParam = key.toString().toLowerCase().includes('pubkey')
                  const formattedParamValue = (key as SolanaInstructionParamKeys === 'lamports'
                    // format lamports to SOL
                    ? new Amount(value.toString()).div(LAMPORTS_PER_SOL).formatAsAsset(9, 'SOL')

                    // show friendly account address names
                    : isAddressParam ? findAccountName(accounts, value.toString()) ?? value

                      // unformatted param value
                      : value
                  ).toString()

                  return <InstructionParamBox key={key}>
                    <var>{paramName}</var>
                    {isAddressParam
                      ? <Tooltip
                        isAddress
                        text={value.toString()}
                        position='left'
                      >
                        <AddressText>{formattedParamValue}</AddressText>
                      </Tooltip>
                      : <samp>{formattedParamValue}</samp>
                    }
                  </InstructionParamBox>
                })
              }
            </>
          )}
        </>
      )
      }
    </InstructionBox >
  )
}

export default SolanaTransactionInstruction
