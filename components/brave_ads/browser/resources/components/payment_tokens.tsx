/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { PaymentToken } from '../lib/app_store'
import { renderCopyableText } from '../lib/copyable_text'
import {
  formatRelativeDuration,
  formatUnixEpochToLocalTime,
  uniqueTableRows,
} from '../lib/format_time'
import { useCopyToClipboard } from './copy_toast'
import { TabHeader } from './tab_header'

const PAYMENT_TOKEN_COLUMNS: Array<keyof PaymentToken> = [
  'Transaction ID',
  'Ad Type',
  'Confirmation Type',
  'Value',
]

// Transaction ID is truncated but a v4 UUID fits comfortably well within
// 30%, so it doesn't need to dominate the row the way `wide-column` would.
const COLUMN_WIDTH_CLASSES: Partial<Record<keyof PaymentToken, string>> = {
  'Transaction ID': 'extra-wide-column',
  'Ad Type': 'value-column',
  'Confirmation Type': 'value-column',
  'Value': 'value-column',
}

// 3 decimal places, avoiding raw floating point noise like
// "0.7100000000000002".
const valueFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 3,
  maximumFractionDigits: 3,
})

function formatValue(value: number | undefined) {
  if (value === undefined) {
    return 'Unknown'
  }
  if (value === 0) {
    return 'No value'
  }
  return valueFormatter.format(value)
}

// The total is always a real, known number (defaults to 0 via `reduce`),
// never "unknown" like an individual row's `Value` can be. A legitimately
// zero total should not read the same as "value not yet known".
function formatTotal(total: number) {
  return valueFormatter.format(total)
}

function PaymentTokensTable({ data }: { data: PaymentToken[] }) {
  const copy = useCopyToClipboard()
  const rows = uniqueTableRows(data)
  const total = rows.reduce((sum, row) => sum + (row['Value'] ?? 0), 0)

  return (
    <>
      <table>
        <thead>
          <tr>
            {PAYMENT_TOKEN_COLUMNS.map((header) => (
              <th
                key={header}
                className={[
                  header === 'Transaction ID' ? 'truncate-cell' : '',
                  COLUMN_WIDTH_CLASSES[header] ?? '',
                ].join(' ').trim()}
              >
                {header === 'Value' ? 'BAT Value' : header}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {rows.map((row, index) => (
            <tr key={index}>
              {PAYMENT_TOKEN_COLUMNS.map((header) => {
                if (header === 'Transaction ID') {
                  return (
                    <td
                      key={header}
                      className={`truncate-cell ${COLUMN_WIDTH_CLASSES[header]}`}
                    >
                      {renderCopyableText(row[header] as string, `${index}`, copy)}
                    </td>
                  )
                }
                return (
                  <td
                    key={header}
                    className={[
                      COLUMN_WIDTH_CLASSES[header] ?? '',
                      header === 'Value' &&
                          (row[header] === undefined || row[header] === 0)
                        ? 'diagnostic-muted'
                        : '',
                    ].join(' ').trim()}
                  >
                    {header === 'Value'
                      ? (
                        <span className='monospace-value'>
                          {formatValue(row[header])}
                        </span>
                      )
                      : row[header]}
                  </td>
                )
              })}
            </tr>
          ))}
        </tbody>
        {rows.length > 0 && (
          <tfoot>
            <tr>
              <td colSpan={PAYMENT_TOKEN_COLUMNS.length - 1}>Total</td>
              <td>
                <span className='monospace-value'>{formatTotal(total)}</span>
              </td>
            </tr>
          </tfoot>
        )}
      </table>
      {data.length === 0 && <p>There are no unredeemed payment tokens.</p>}
    </>
  )
}

export function PaymentTokens() {
  const actions = useAppActions()
  const paymentTokens = useAppState((state) => state.paymentTokens)
  const nextPaymentTokenRedemptionAt =
    useAppState((state) => state.nextPaymentTokenRedemptionAt)

  React.useEffect(() => {
    actions.loadAdsInternals()
  }, [])

  return (
    <div className='card-group'>
      <TabHeader
        title={
          <>
            Payment tokens{' '}
            <span className='diagnostic-muted'>({paymentTokens.length})</span>
          </>
        }
        description='Payment tokens earned by showing ads, not yet redeemed
          for BAT.'
        onRefresh={actions.loadAdsInternals}
      >
        <p>
          {nextPaymentTokenRedemptionAt
            ? <>
                Payment tokens will next be redeemed on{' '}
                {formatUnixEpochToLocalTime(nextPaymentTokenRedemptionAt)}{' '}
                <span className='diagnostic-muted'>
                  ({formatRelativeDuration(nextPaymentTokenRedemptionAt)})
                </span>.
              </>
            : 'Redemption is not yet scheduled.'}
        </p>
      </TabHeader>

      <div className='content-card'>
        <section>
          <PaymentTokensTable data={paymentTokens} />
        </section>
      </div>
    </div>
  )
}
