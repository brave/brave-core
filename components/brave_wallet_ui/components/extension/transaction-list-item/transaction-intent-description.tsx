import * as React from 'react'
import { DetailTextLight } from '../shared-panel-styles'
import { ArrowIcon, DetailWrappedText } from './style'

interface Props {
  to: string
  from: string
  wrapFrom?: boolean
}

export const TransactionIntentDescription = ({ from, to, wrapFrom }: Props): JSX.Element => {
  const fromText = <DetailTextLight>{from}</DetailTextLight>
  return <>
    {wrapFrom ? <DetailWrappedText>{fromText}</DetailWrappedText> : fromText}
    {' '}<ArrowIcon />
    <DetailTextLight>{to}</DetailTextLight>
  </>
}
