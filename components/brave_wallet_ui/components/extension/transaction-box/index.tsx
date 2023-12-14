// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../../constants/types'
import {
  TypedSolanaInstructionWithParams //
} from '../../../utils/solana-instruction-utils'

// utils
import { getLocale } from '../../../../common/locale'
import { numberArrayToHexStr } from '../../../utils/hex-utils'

// components
import {
  SolanaTransactionInstruction //
} from '../../shared/solana-transaction-instruction/solana-transaction-instruction'

// style
import {
  BitcoinDetailColumn,
  BitcoinDetailLine,
  CodeSnippet,
  CodeSnippetText,
  DetailColumn,
  DetailText,
  TransactionText
} from './style'

export interface Props {
  transactionInfo: SerializableTransactionInfo
  instructions?: TypedSolanaInstructionWithParams[]
}

const txKeys = Object.keys(BraveWallet.TransactionType)

export const TransactionDetailBox = ({
  transactionInfo,
  instructions
}: Props) => {
  const { txArgs, txParams, txType, txDataUnion } = transactionInfo

  const solData = txDataUnion.solanaTxData
  const btcData = txDataUnion.btcTxData
  const zecData = txDataUnion.zecTxData
  const dataArray = txDataUnion.ethTxData1559?.baseData.data || []

  // render
  // No Data
  if (dataArray.length === 0 && !btcData && !zecData && !solData) {
    return (
      <CodeSnippet>
        <code>
          <CodeSnippetText>
            {getLocale('braveWalletConfirmTransactionNoData')}
          </CodeSnippetText>
        </code>
      </CodeSnippet>
    )
  }

  // BTC
  // TODO(apaymyshev): strings localization.
  if (btcData) {
    return (
      <BitcoinDetailColumn>
        {btcData.inputs?.map((input, index) => {
          return (
            <div key={'input' + index}>
              <BitcoinDetailLine>{`Input: ${index}`}</BitcoinDetailLine>
              <BitcoinDetailLine>{`Value: ${input.value}`}</BitcoinDetailLine>
              <BitcoinDetailLine>{`Address: ${
                input.address //
              }`}</BitcoinDetailLine>
            </div>
          )
        })}
        {btcData.outputs?.map((output, index) => {
          return (
            <div key={'output' + index}>
              <BitcoinDetailLine>{`Output: ${index}`}</BitcoinDetailLine>
              <BitcoinDetailLine>{`Value: ${output.value}`}</BitcoinDetailLine>
              <BitcoinDetailLine>
                {`Address: ${output.address}`}
              </BitcoinDetailLine>
            </div>
          )
        })}
      </BitcoinDetailColumn>
    )
  }

  // ZEC
  if (zecData) {
    return (
      <>
        <DetailColumn>
          {zecData.inputs?.map((input, index) => {
            return (
              <code key={index}>{`input-${input.value}-${input.address}`}</code>
            )
          })}
        </DetailColumn>

        <DetailColumn>
          {zecData.outputs?.map((output, index) => {
            return (
              <code
                key={index}
              >{`output-${output.value}-${output.address}`}</code>
            )
          })}
        </DetailColumn>
      </>
    )
  }

  // SOL, EVM & FIL
  return (
    <>
      {solData || dataArray ? (
        <DetailColumn>
          <TransactionText>
            {getLocale('braveWalletTransactionDetailBoxFunction')}:
          </TransactionText>
          <DetailText>{txKeys[txType]}</DetailText>
        </DetailColumn>
      ) : null}

      {
        // SOL
        instructions?.length ? (
          <DetailColumn>
            {instructions?.map((instruction, index) => {
              return (
                <SolanaTransactionInstruction
                  key={index}
                  typedInstructionWithParams={instruction}
                />
              )
            })}
          </DetailColumn>
        ) : // FIL & EVM
        txType === BraveWallet.TransactionType.Other ? (
          <CodeSnippet>
            <code>
              <CodeSnippetText>
                {`0x${numberArrayToHexStr(dataArray)}`}
              </CodeSnippetText>
            </code>
          </CodeSnippet>
        ) : (
          txParams.map((param, i) => (
            <CodeSnippet key={i}>
              <code>
                <CodeSnippetText>
                  {param}: {txArgs[i]}
                </CodeSnippetText>
              </code>
            </CodeSnippet>
          ))
        )
      }
    </>
  )
}

export default TransactionDetailBox
