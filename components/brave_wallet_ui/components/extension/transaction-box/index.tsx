// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '../../../../common/locale'
import { numberArrayToHexStr } from '../../../utils/hex-utils'
import { BraveWallet, SerializableTransactionInfo } from '../../../constants/types'
import { CodeSnippet, CodeSnippetText, DetailColumn, DetailText, TransactionText } from './style'

export interface Props {
  transactionInfo: SerializableTransactionInfo
}

const txKeys = Object.keys(BraveWallet.TransactionType)

const TransactionDetailBox = (props: Props) => {
  const { transactionInfo } = props
  const {
    txArgs,
    txParams,
    txType
  } = transactionInfo
  const data = transactionInfo.txDataUnion.ethTxData1559?.baseData.data || []
  return (
    <>
      {data.length === 0 ? (
        <CodeSnippet>
          <code>
            <CodeSnippetText>{getLocale('braveWalletConfirmTransactionNoData')}</CodeSnippetText>
          </code>
        </CodeSnippet>
      ) : (
        <>
          <DetailColumn>
            <TransactionText>{getLocale('braveWalletTransactionDetailBoxFunction')}:</TransactionText>
            <DetailText>{txKeys[txType]}</DetailText>
          </DetailColumn>
          {txType !== BraveWallet.TransactionType.Other && txParams.map((param, i) =>
            <CodeSnippet key={i}>
              <code>
                <CodeSnippetText>{param}: {txArgs[i]}</CodeSnippetText>
              </code>
            </CodeSnippet>
          )}

          {txType === BraveWallet.TransactionType.Other && (
            <CodeSnippet>
              <code>
                <CodeSnippetText>
                  {`0x${numberArrayToHexStr(data)}`}
                </CodeSnippetText>
              </code>
            </CodeSnippet>
          )}
        </>
      )}
    </>
  )
}

export default TransactionDetailBox
