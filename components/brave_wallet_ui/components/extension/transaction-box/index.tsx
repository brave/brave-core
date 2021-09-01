import * as React from 'react'
import {
  CodeSnippet,
  CodeSnippetText
} from './style'

export interface Props {
  transactionData: string[]
  hasNoData: boolean
}

const TransactionDetailBox = (props: Props) => {
  const { transactionData, hasNoData } = props
  return (
    <>
      {hasNoData ? (
        <CodeSnippet>
          <code>
            <CodeSnippetText>No Data.</CodeSnippetText>
          </code>
        </CodeSnippet>
      ) : (
        <>
          {transactionData.map((data, i) =>
            <CodeSnippet key={i}>
              <code>
                <CodeSnippetText>{data}</CodeSnippetText>
              </code>
            </CodeSnippet>
          )}
        </>
      )}
    </>
  )
}

export default TransactionDetailBox
