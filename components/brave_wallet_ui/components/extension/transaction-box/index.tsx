import * as React from 'react'
import { TransactionDataType } from '../../../constants/types'
import locale from '../../../constants/locale'
// Styled Components
import {
  TransactionText,
  HexBlock,
  DetailRow,
  DetailText,
  CodeSnippet,
  CodeSnippetText
} from './style'

export interface Props {
  transactionData: TransactionDataType
}

const TransactionDetailBox = (props: Props) => {
  const { transactionData } = props
  return (
    <>
      <DetailRow>
        <TransactionText>{locale.transactionDetailBoxFunction}:</TransactionText>
        <DetailText>{transactionData.functionName}</DetailText>
      </DetailRow>
      <CodeSnippet>
        <code>
          <CodeSnippetText>{transactionData.parameters}</CodeSnippetText>
        </code>
      </CodeSnippet>
      <DetailRow>
        <TransactionText>{locale.transactionDetailBoxHex}:</TransactionText>
        <DetailText>{transactionData.hexSize} {locale.transactionDetailBoxBytes}</DetailText>
      </DetailRow>
      <HexBlock>{transactionData.hexData}</HexBlock>
    </>
  )
}

export default TransactionDetailBox
