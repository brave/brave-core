// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import {
  BraveWallet,
  SerializableTransactionInfo,
} from '../../../constants/types'
import {
  TypedSolanaInstructionWithParams, //
} from '../../../utils/solana-instruction-utils'

// utils
import { getLocale } from '../../../../common/locale'
import { numberArrayToHexStr } from '../../../utils/hex-utils'
import {
  getTransactionTypeName,
  isBitcoinTransaction,
  isCardanoTransaction,
  isZCashTransaction,
} from '../../../utils/tx-utils'

// components
import {
  SolanaTransactionInstruction, //
} from '../../shared/solana-transaction-instruction/solana-transaction-instruction'

// style
import {
  CodeDetailLine,
  CodeSnippet,
  CodeSnippetText,
  DetailColumn,
  DetailText,
  TransactionText,
} from './style'

export interface Props {
  transactionInfo: SerializableTransactionInfo
  instructions?: TypedSolanaInstructionWithParams[]
}

function BtcTransactionDetails({
  btcTxData,
}: {
  btcTxData: BraveWallet.BtcTxData
}) {
  return (
    <DetailColumn>
      {btcTxData.inputs?.map((input, index) => (
        <div key={'input' + index}>
          <CodeDetailLine>{`Input: ${index}`}</CodeDetailLine>
          <CodeDetailLine>{`Value: ${input.value}`}</CodeDetailLine>
          <CodeDetailLine>{`Address: ${input.address}`}</CodeDetailLine>
        </div>
      ))}
      {btcTxData.outputs?.map((output, index) => (
        <div key={'output' + index}>
          <CodeDetailLine>{`Output: ${index}`}</CodeDetailLine>
          <CodeDetailLine>{`Value: ${output.value}`}</CodeDetailLine>
          <CodeDetailLine>{`Address: ${output.address}`}</CodeDetailLine>
        </div>
      ))}
    </DetailColumn>
  )
}

function CardanoTransactionDetails({
  cardanoTxData,
}: {
  cardanoTxData: BraveWallet.CardanoTxData
}) {
  return (
    <DetailColumn>
      {cardanoTxData.inputs?.map((input, index) => (
        <div key={'input' + index}>
          <CodeDetailLine>{`Input: ${index}`}</CodeDetailLine>
          <CodeDetailLine>{`Value: ${input.value}`}</CodeDetailLine>
          {input.tokens.map((token) => {
            return (
              <CodeDetailLine
                key={token.tokenIdHex}
              >{`Token: ${token.tokenIdHex}, Value: ${token.value}`}</CodeDetailLine>
            )
          })}
          <CodeDetailLine>{`Address: ${input.address}`}</CodeDetailLine>
        </div>
      ))}
      {cardanoTxData.outputs?.map((output, index) => (
        <div key={'output' + index}>
          <CodeDetailLine>{`Output: ${index}`}</CodeDetailLine>
          <CodeDetailLine>{`Value: ${output.value}`}</CodeDetailLine>
          {output.tokens.map((token) => {
            return (
              <CodeDetailLine
                key={token.tokenIdHex}
              >{`Token: ${token.tokenIdHex}, Value: ${token.value}`}</CodeDetailLine>
            )
          })}
          <CodeDetailLine>{`Address: ${output.address}`}</CodeDetailLine>
        </div>
      ))}
    </DetailColumn>
  )
}

function ZecTransactionDetails({
  zecTxData,
}: {
  zecTxData: BraveWallet.ZecTxData
}) {
  return (
    <>
      <DetailColumn>
        {zecTxData.inputs?.map((input, index) => (
          <CodeDetailLine
            key={index}
          >{`input-${input.value}-${input.address}`}</CodeDetailLine>
        ))}
      </DetailColumn>
      <DetailColumn>
        {zecTxData.outputs?.map((output, index) => (
          <CodeDetailLine
            key={index}
          >{`output-${output.value}-${output.address}`}</CodeDetailLine>
        ))}
      </DetailColumn>
    </>
  )
}

export const TransactionDetailBox = ({
  transactionInfo,
  instructions,
}: Props) => {
  const { txArgs, txParams, txType, txDataUnion } = transactionInfo

  const solData = txDataUnion.solanaTxData
  const sendOptions = solData?.sendOptions

  const dataArray = txDataUnion.ethTxData1559?.baseData.data || []

  // BTC
  if (isBitcoinTransaction(transactionInfo)) {
    return (
      <BtcTransactionDetails
        btcTxData={transactionInfo.txDataUnion.btcTxData}
      />
    )
  }

  // Cardano
  if (isCardanoTransaction(transactionInfo)) {
    return (
      <CardanoTransactionDetails
        cardanoTxData={transactionInfo.txDataUnion.cardanoTxData}
      />
    )
  }

  // ZEC
  if (isZCashTransaction(transactionInfo)) {
    return (
      <ZecTransactionDetails
        zecTxData={transactionInfo.txDataUnion.zecTxData}
      />
    )
  }

  // No Data
  if (dataArray.length === 0 && !solData) {
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
