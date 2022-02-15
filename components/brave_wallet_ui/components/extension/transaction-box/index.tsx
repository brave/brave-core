import * as React from 'react'

import { getLocale } from '../../../../common/locale'
import { numberArrayToHexStr } from '../../../utils/hex-utils'
import { BraveWallet } from '../../../constants/types'
import { CodeSnippet, CodeSnippetText, DetailRow, DetailText, TransactionText } from './style'

export interface Props {
  transactionInfo: BraveWallet.TransactionInfo
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
          <DetailRow>
            <TransactionText>{getLocale('braveWalletTransactionDetailBoxFunction')}:</TransactionText>
            <DetailText>{txKeys[txType]}</DetailText>
          </DetailRow>
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
