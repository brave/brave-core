// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'

// types
import { BraveWallet } from '../../../constants/types'

import {
  CodeSnippet,
  CodeSnippetText,
  DetailColumn,
} from './style'

interface Props {
  data: BraveWallet.ZecTxData | undefined
}

// TODO(cypt4): better design for this
export const ZCashTransactionDetailBox = ({
  data,
}: Props) => {
  if (!data) {
    return (
      <CodeSnippet>
        <code>
          <CodeSnippetText>{getLocale('braveWalletConfirmTransactionNoData')}</CodeSnippetText>
        </code>
      </CodeSnippet>
    )
  }

  return (
    <>
      {
        <DetailColumn>
          {data.inputs?.map((input, index) => {
            return <code key={index}>{`input-${input.value}-${input.address}`}</code>
          })}
        </DetailColumn>
      }
      {
        <DetailColumn>
          {data.outputs?.map((output, index) => {
            return <code key={index}>{`output-${output.value}-${output.address}`}</code>
          })}
        </DetailColumn>
      }
    </>
  )
}
