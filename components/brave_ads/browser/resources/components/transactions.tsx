/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { Transaction } from '../lib/app_store'
import { renderCopyableText } from '../lib/copyable_text'
import { formatUnixEpochToLocalTime, uniqueTableRows } from '../lib/format_time'
import { useCopyToClipboard } from './copy_toast'
import { computeDateRange, DateRangeFilter } from './date_range_filter'
import { TabHeader } from './tab_header'

const TRANSACTION_COLUMNS: Array<keyof Transaction> = [
  'Transaction ID',
  'Creative Instance ID',
  'Ad Type',
  'Confirmation Type',
  'Created At',
  'Reconciled At',
  'Value',
]

const TRUNCATED_COLUMNS = new Set<keyof Transaction>([
  'Transaction ID',
  'Creative Instance ID',
])

// Explicit widths so the two tables' shared columns line up visually even
// though the "Not yet reconciled" table drops "Reconciled At" entirely; its
// "Created At" absorbs that column's width instead of the columns shifting.
const COLUMN_WIDTH_CLASSES: Partial<Record<keyof Transaction, string>> = {
  'Transaction ID': 'value-column',
  'Creative Instance ID': 'value-column',
  'Ad Type': 'narrow-value-column',
  'Confirmation Type': 'narrow-value-column',
  'Created At': 'status-column',
  'Reconciled At': 'status-column',
  'Value': 'status-column',
}
const CREATED_AT_WITHOUT_RECONCILED_AT_CLASS = 'value-column'

// 3 decimal places, avoiding raw floating point noise like
// "0.7100000000000002".
const valueFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 3,
  maximumFractionDigits: 3,
})

function TransactionsSection({ title, data, showReconciledAt }: {
  title: string
  data: Transaction[]
  showReconciledAt: boolean
}) {
  return (
    <div className='content-card'>
      <h4>
        <span className='title'>
          {title} <span className='diagnostic-muted'>({data.length})</span>
        </span>
      </h4>
      <section>
        <TransactionsTable data={data} showReconciledAt={showReconciledAt} />
      </section>
    </div>
  )
}

function TransactionsTable(
  { data, showReconciledAt }: { data: Transaction[], showReconciledAt: boolean },
) {
  const copy = useCopyToClipboard()
  const rows = uniqueTableRows(data)
  const total = rows.reduce((sum, row) => sum + row['Value'], 0)
  // "Reconciled At" is always "N/A" for not-yet-reconciled transactions, so
  // that section omits the column entirely rather than showing a column of
  // nothing but "N/A".
  const columns = showReconciledAt
    ? TRANSACTION_COLUMNS
    : TRANSACTION_COLUMNS.filter((header) => header !== 'Reconciled At')

  function widthClassName(header: keyof Transaction) {
    if (header === 'Created At' && !showReconciledAt) {
      return CREATED_AT_WITHOUT_RECONCILED_AT_CLASS
    }
    return COLUMN_WIDTH_CLASSES[header] ?? ''
  }

  return (
    <>
      <table>
        <thead>
          <tr>
            {columns.map((header) => (
              <th
                key={header}
                className={[
                  TRUNCATED_COLUMNS.has(header) ? 'truncate-cell' : '',
                  widthClassName(header),
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
              {columns.map((header) => {
                if (TRUNCATED_COLUMNS.has(header)) {
                  return (
                    <td
                      key={header}
                      className={`truncate-cell ${widthClassName(header)}`}
                    >
                      {renderCopyableText(row[header] as string, `${index}`, copy)}
                    </td>
                  )
                }
                if (header === 'Value') {
                  return (
                    <td key={header} className={widthClassName(header)}>
                      <span className='monospace-value'>
                        {valueFormatter.format(row.Value)}
                      </span>
                    </td>
                  )
                }
                if (header === 'Created At' || header === 'Reconciled At') {
                  const value = row[header]
                  return (
                    <td key={header} className={widthClassName(header)}>
                      {value === undefined
                        ? <span className='diagnostic-muted'>N/A</span>
                        : formatUnixEpochToLocalTime(value)}
                    </td>
                  )
                }
                return (
                  <td key={header} className={widthClassName(header)}>
                    {row[header]}
                  </td>
                )
              })}
            </tr>
          ))}
        </tbody>
        {rows.length > 0 && (
          <tfoot>
            <tr>
              <td colSpan={columns.length - 1}>Total</td>
              <td>
                <span className='monospace-value'>
                  {valueFormatter.format(total)}
                </span>
              </td>
            </tr>
          </tfoot>
        )}
      </table>
      {data.length === 0 && <p>No transactions match the selected date range.</p>}
    </>
  )
}

export function Transactions() {
  const actions = useAppActions()
  const transactions = useAppState((state) => state.transactions)
  const dateRangeFilter =
    useAppState((state) => state.transactionsDateRangeFilter)
  const { preset, fromDate, toDate } = dateRangeFilter

  React.useEffect(() => {
    actions.loadAdsInternals()
  }, [])

  const { fromSeconds, toSeconds } = computeDateRange(preset, fromDate, toDate)

  // Defaults to showing every transaction; the date inputs narrow the range
  // only once the user picks one.
  const { filtered, notReconciled, reconciled } = React.useMemo(() => {
    const filteredTransactions = transactions.filter((transaction) => {
      const createdAt = transaction['Created At']
      if (createdAt === undefined) {
        return true
      }
      if (fromSeconds !== null && createdAt < fromSeconds) {
        return false
      }
      if (toSeconds !== null && createdAt > toSeconds) {
        return false
      }
      return true
    })

    return {
      filtered: filteredTransactions,
      notReconciled:
        filteredTransactions.filter((transaction) =>
          transaction['Reconciled At'] === undefined),
      reconciled:
        filteredTransactions.filter((transaction) =>
          transaction['Reconciled At'] !== undefined),
    }
  }, [transactions, fromSeconds, toSeconds])

  return (
    <div className='card-group'>
      <TabHeader
        title={
          <>
            Transactions{' '}
            <span className='diagnostic-muted'>({filtered.length})</span>
          </>
        }
        description='Transactions recorded for served ads; reconciled for
          BAT once redeemed.'
        onRefresh={actions.loadAdsInternals}
      >
        <DateRangeFilter
          filter={dateRangeFilter}
          setFilter={actions.setTransactionsDateRangeFilter}
          defaultPreset='day'
        />
      </TabHeader>

      <TransactionsSection
        title='Not yet reconciled'
        data={notReconciled}
        showReconciledAt={false}
      />
      <TransactionsSection
        title='Reconciled'
        data={reconciled}
        showReconciledAt={true}
      />
    </div>
  )
}
