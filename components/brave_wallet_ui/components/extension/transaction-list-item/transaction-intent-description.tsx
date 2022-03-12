import * as React from 'react'
import { DetailTextLight } from '../shared-panel-styles'
import { ArrowIcon } from './style'

interface Props {
  to: string
  from: string
}

export function TransactionIntentDescription ({ from, to }: Props): JSX.Element {
  return <>
    {' '}
    <DetailTextLight>{to}</DetailTextLight>
    {' '}<ArrowIcon />{' '}
    <DetailTextLight>{from}</DetailTextLight>
    {' '}
  </>
}
