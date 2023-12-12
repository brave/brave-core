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
  BitcoinDetailColumn,
  BitcoinDetailLine,
  CodeSnippet,
  CodeSnippetText
} from './style'

interface Props {
  data: BraveWallet.BtcTxData | undefined
}

// TODO(apaymyshev): better design for this
// TODO(apaymyshev): strings localization.
export const BitcoinTransactionDetailBox = ({ data }: Props) => {
  if (!data) {
    return (
      <CodeSnippet>
        <code>
          <CodeSnippetText>
            {getLocale('braveWalletConfirmTransactionNoData')}
          </CodeSnippetText>
        </code>
      </CodeSnippet>
    )
  }

  return (
    <>
      {
        <BitcoinDetailColumn>
          {data.inputs?.map((input, index) => {
            return (
              <div key={'input' + index}>
                <BitcoinDetailLine>{`Input: ${index}`}</BitcoinDetailLine>
                <BitcoinDetailLine>{`Value: ${input.value}`}</BitcoinDetailLine>
                <BitcoinDetailLine>
                  {`Address: ${input.address}`}
                </BitcoinDetailLine>
              </div>
            )
          })}
          {data.outputs?.map((output, index) => {
            return (
              <div key={'output' + index}>
                <BitcoinDetailLine>{`Output: ${index}`}</BitcoinDetailLine>
                <BitcoinDetailLine>
                  {`Value: ${output.value}`}
                </BitcoinDetailLine>
                <BitcoinDetailLine>
                  {`Address: ${output.address}`}
                </BitcoinDetailLine>
              </div>
            )
          })}
        </BitcoinDetailColumn>
      }
    </>
  )
}
