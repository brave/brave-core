// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
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
