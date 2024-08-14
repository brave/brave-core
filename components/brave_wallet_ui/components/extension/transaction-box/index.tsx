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
import { getTransactionTypeName } from '../../../utils/tx-utils'

// components
import {
  SolanaTransactionInstruction //
} from '../../shared/solana-transaction-instruction/solana-transaction-instruction'

// style
import {
  BitcoinDetailColumn,
  CodeDetailLine,
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

export const TransactionDetailBox = ({
  transactionInfo,
  instructions
}: Props) => {
  const { txArgs, txParams, txType, txDataUnion } = transactionInfo

  const solData = txDataUnion.solanaTxData
  const sendOptions = solData?.sendOptions

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
              <CodeDetailLine>{`Input: ${index}`}</CodeDetailLine>
              <CodeDetailLine>{`Value: ${input.value}`}</CodeDetailLine>
              <CodeDetailLine>{`Address: ${
                input.address //
              }`}</CodeDetailLine>
            </div>
          )
        })}
        {btcData.outputs?.map((output, index) => {
          return (
            <div key={'output' + index}>
              <CodeDetailLine>{`Output: ${index}`}</CodeDetailLine>
              <CodeDetailLine>{`Value: ${output.value}`}</CodeDetailLine>
              <CodeDetailLine>{`Address: ${output.address}`}</CodeDetailLine>
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
              <CodeDetailLine
                key={index}
              >{`input-${input.value}-${input.address}`}</CodeDetailLine>
            )
          })}
        </DetailColumn>

        <DetailColumn>
          {zecData.outputs?.map((output, index) => {
            return (
              <CodeDetailLine
                key={index}
              >{`output-${output.value}-${output.address}`}</CodeDetailLine>
            )
          })}
        </DetailColumn>
      </>
    )
  }

  // SOL, EVM & FIL
  return (
    <>
      {sendOptions && (
        <>
          {!!Number(sendOptions?.maxRetries?.maxRetries) && (
            <DetailColumn key={'maxRetries'}>
              <TransactionText>
                {getLocale('braveWalletSolanaMaxRetries')}:
              </TransactionText>
              <DetailText>{sendOptions?.maxRetries?.maxRetries}</DetailText>
            </DetailColumn>
          )}

          {sendOptions?.preflightCommitment && (
            <DetailColumn key={'preflightCommitment'}>
              <TransactionText>
                {getLocale('braveWalletSolanaPreflightCommitment')}:
              </TransactionText>
              <DetailText>{sendOptions?.preflightCommitment}</DetailText>
            </DetailColumn>
          )}

          {sendOptions?.skipPreflight && (
            <DetailColumn key={'skipPreflight'}>
              <TransactionText>
                {getLocale('braveWalletSolanaSkipPreflight')}:
              </TransactionText>
              <DetailText>
                {sendOptions.skipPreflight.skipPreflight.toString()}
              </DetailText>
            </DetailColumn>
          )}
        </>
      )}

      {solData || dataArray ? (
        <DetailColumn>
          <TransactionText>
            {getLocale('braveWalletTransactionDetailBoxFunction')}:
          </TransactionText>
          <DetailText>{getTransactionTypeName(txType)}</DetailText>
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
