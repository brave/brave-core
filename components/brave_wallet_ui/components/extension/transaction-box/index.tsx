import * as React from 'react'

import { getLocale } from '../../../../common/locale'
import { uint8ArrayToHexStr } from '../../../utils/hex-utils'
import { TransactionInfo, TransactionType } from '../../../constants/types'
import { CodeSnippet, CodeSnippetText, DetailRow, DetailText, TransactionText } from './style'

export interface Props {
  transactionInfo: TransactionInfo
}

const TransactionDetailBox = (props: Props) => {
  const { transactionInfo } = props
  const {
    txData: {
      baseData: { data }
    },
    txArgs,
    txParams,
    txType
  } = transactionInfo

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
            <DetailText>{TransactionType[txType]}</DetailText>
          </DetailRow>
          {txType !== TransactionType.Other && txParams.map((param, i) =>
            <CodeSnippet key={i}>
              <code>
                <CodeSnippetText>{param}: {txArgs[i]}</CodeSnippetText>
              </code>
            </CodeSnippet>
          )}

          {txType === TransactionType.Other && (
            <CodeSnippet>
              <code>
                <CodeSnippetText>
                  {`0x${uint8ArrayToHexStr(data)}`}
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
