/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../../../common/locale'

interface Props {
  paymentId: string
  logEntries: RewardsInternals.EventLog[]
}

function processLogEntries (
  currentPaymentId: string,
  entries: RewardsInternals.EventLog[]
) {
  interface Result {
    paymentId: string
    corrupted: boolean
    timestamp: number
  }

  entries = entries
    .sort((a, b) => b.createdAt - a.createdAt)
    .filter((entry) => {
      switch (entry.key) {
        case 'wallet.payment_id':
        case 'wallet_corrupted':
          return true
        default:
          return false
      }
    })

  const list: Result[] = []
  let corrupted = false

  for (const { key, value, createdAt } of entries) {
    switch (key) {
      case 'wallet.payment_id':
        if (value !== currentPaymentId) {
          list.push({
            paymentId: value,
            timestamp: createdAt * 1000,
            corrupted
          })
        }
        corrupted = false
        break
      case 'wallet_corrupted':
        corrupted = true
        break
    }
  }

  return list
}

export function WalletHistory (props: Props) {
  const results = processLogEntries(props.paymentId, props.logEntries)
  if (results.length === 0) {
    return null
  }

  const dateFormatter = new Intl.DateTimeFormat(undefined, {
    month: '2-digit',
    day: '2-digit',
    year: 'numeric'
  })

  return (
    <div>
      <h3>{getLocale('walletHistory')}</h3>
      {
        results.map((result) =>
          <div key={result.paymentId}>
            {result.paymentId}{result.corrupted ? '*' : ''}&nbsp;
            ({dateFormatter.format(new Date(result.timestamp))})
          </div>
        )
      }
    </div>
  )
}
