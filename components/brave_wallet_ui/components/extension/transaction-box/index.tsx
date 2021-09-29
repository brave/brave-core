import * as React from 'react'
import { TransactionInfo, TransactionType } from '../../../constants/types'
import locale from '../../../constants/locale'
import {
  CodeSnippet,
  CodeSnippetText,
  DetailRow,
  TransactionText,
  DetailText
} from './style'

export interface Props {
  transactionInfo: TransactionInfo
  hasNoData: boolean
}

const TransactionDetailBox = (props: Props) => {
  const { transactionInfo, hasNoData } = props
  const { txArgs, txParams, txType } = transactionInfo
  return (
    <>
      {hasNoData ? (
        <CodeSnippet>
          <code>
            <CodeSnippetText>{locale.confirmTransactionNoData}</CodeSnippetText>
          </code>
        </CodeSnippet>
      ) : (
        <>
          <DetailRow>
            <TransactionText>{locale.transactionDetailBoxFunction}:</TransactionText>
            <DetailText>{TransactionType[txType]}</DetailText>
          </DetailRow>
          {txParams.map((data, i) =>
            <CodeSnippet key={i}>
              <code>
                <CodeSnippetText>{data}: {txArgs[i]}</CodeSnippetText>
              </code>
            </CodeSnippet>
          )}
        </>
      )}
    </>
  )
}

export default TransactionDetailBox
