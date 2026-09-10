/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { ConfirmationQueueItem } from '../lib/app_store'
import { renderCopyableText } from '../lib/copyable_text'
import {
  formatRelativeDuration,
  formatUnixEpochToLocalTime,
  nowInSeconds,
  uniqueTableRows,
} from '../lib/format_time'
import { useCopyToClipboard } from './copy_toast'
import { JsonBlock } from './json_block'
import { TabHeader } from './tab_header'

const CONFIRMATION_QUEUE_COLUMNS: Array<keyof ConfirmationQueueItem> = [
  'Transaction ID',
  'Ad Type',
  'Confirmation Type',
  'Retry Count',
  'Created At',
  'Process At',
  'User Data',
]

// Only "Transaction ID" is truncated (a UUID, meaningful even clipped, with
// a copy tooltip). The timestamp columns wrap onto multiple lines instead,
// since a clipped date/time string isn't meaningful on its own, and "User
// Data" is pretty-printed JSON (see `JsonBlock`) that needs to actually
// expand, not truncate.
const TRUNCATED_COLUMNS = new Set<keyof ConfirmationQueueItem>([
  'Transaction ID',
])

const ONE_DAY_SECONDS = 24 * 60 * 60
const STUCK_ORANGE_AFTER_SECONDS = ONE_DAY_SECONDS
const STUCK_RED_AFTER_SECONDS = 3 * ONE_DAY_SECONDS

// Retries back off exponentially (each retry roughly doubles the delay before
// the next attempt), so a handful of retries is normal transient-failure
// recovery, not yet a sign anything is actually wrong.
const RETRY_ORANGE_AFTER_COUNT = 3
const RETRY_RED_AFTER_COUNT = 5

type Severity = '' | 'confirmation-stuck-orange' | 'confirmation-stuck-red'

function moreSevere(a: Severity, b: Severity): Severity {
  if (a === 'confirmation-stuck-red' || b === 'confirmation-stuck-red') {
    return 'confirmation-stuck-red'
  }
  if (a === 'confirmation-stuck-orange' || b === 'confirmation-stuck-orange') {
    return 'confirmation-stuck-orange'
  }
  return ''
}

// How long a confirmation has sat in the queue unresolved, measured from when
// it was created (not from its next scheduled retry); the longer it's been
// stuck, the more likely something is actually wrong rather than just waiting
// for its next attempt.
function ageSeverity(createdAt: number | undefined): Severity {
  if (createdAt === undefined) {
    return ''
  }

  const ageSeconds = nowInSeconds() - createdAt
  if (ageSeconds > STUCK_RED_AFTER_SECONDS) {
    return 'confirmation-stuck-red'
  }
  if (ageSeconds > STUCK_ORANGE_AFTER_SECONDS) {
    return 'confirmation-stuck-orange'
  }
  return ''
}

function retryCountSeverity(retryCount: number): Severity {
  if (retryCount >= RETRY_RED_AFTER_COUNT) {
    return 'confirmation-stuck-red'
  }
  if (retryCount >= RETRY_ORANGE_AFTER_COUNT) {
    return 'confirmation-stuck-orange'
  }
  return ''
}

function stuckRowClassName(row: ConfirmationQueueItem) {
  return moreSevere(
    ageSeverity(row['Created At']),
    retryCountSeverity(row['Retry Count']),
  )
}

function ConfirmationQueueTable({ data }: { data: ConfirmationQueueItem[] }) {
  const copy = useCopyToClipboard()

  return (
    <>
      <table>
        <thead>
          <tr>
            {CONFIRMATION_QUEUE_COLUMNS.map((header) => (
              <th
                key={header}
                className={TRUNCATED_COLUMNS.has(header) ? 'truncate-cell' : ''}
              >
                {header}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {uniqueTableRows(data).map((row, index) => (
            <tr
              key={index}
              className={stuckRowClassName(row)}
            >
              {CONFIRMATION_QUEUE_COLUMNS.map((header) => {
                if (header === 'Transaction ID') {
                  return (
                    <td key={header} className='truncate-cell'>
                      {renderCopyableText(row[header], `${index}`, copy)}
                    </td>
                  )
                }
                if (header === 'User Data') {
                  const userData = row[header]
                  return (
                    <td key={header}>
                      {userData && (
                        <JsonBlock
                          value={userData}
                          keyPrefix={`${index}-user-data`}
                          onCopy={copy}
                        />
                      )}
                    </td>
                  )
                }
                const value = row[header]
                const isTimestamp = header === 'Created At' ||
                  header === 'Process At'
                const isTruncated = TRUNCATED_COLUMNS.has(header)
                const displayValue = isTimestamp
                  ? (value === undefined
                      ? 'N/A'
                      : header === 'Process At'
                        ? <>
                            {formatUnixEpochToLocalTime(value as number)}{' '}
                            <span className='diagnostic-muted'>
                              ({formatRelativeDuration(value as number)})
                            </span>
                          </>
                        : formatUnixEpochToLocalTime(value as number))
                  : value
                return (
                  <td
                    key={header}
                    className={isTruncated ? 'truncate-cell' : ''}
                    title={isTruncated ? String(displayValue) : undefined}
                  >
                    {displayValue}
                  </td>
                )
              })}
            </tr>
          ))}
        </tbody>
      </table>
      {data.length === 0 && <p>The confirmation queue is currently empty.</p>}
    </>
  )
}

export function ConfirmationQueue() {
  const actions = useAppActions()
  const confirmationQueue = useAppState((state) => state.confirmationQueue)

  React.useEffect(() => {
    actions.loadAdsInternals()
  }, [])

  return (
    <div className='card-group'>
      <TabHeader
        title={
          <>
            Confirmation queue{' '}
            <span className='diagnostic-muted'>
              ({confirmationQueue.length})
            </span>
          </>
        }
        description='Ad confirmations waiting to be redeemed. Failed
          attempts are retried automatically, waiting longer between each
          attempt.'
        onRefresh={actions.loadAdsInternals}
      />

      <div className='content-card'>
        <section>
          <ConfirmationQueueTable data={confirmationQueue} />
        </section>
      </div>
    </div>
  )
}
