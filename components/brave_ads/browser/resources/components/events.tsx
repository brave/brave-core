/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useAppState, useAppActions } from '../lib/app_context'
import { AdEvent } from '../lib/app_store'
import { formatUnixEpochToLocalTime, uniqueTableRows } from '../lib/format_time'

const AD_EVENT_COLUMNS: Array<keyof AdEvent> = [
  'Target URL',
  'Ad Type',
  'Event Type',
  'Created At',
]

function AdEventTable({ data }: { data: AdEvent[] }) {
  if (data.length === 0) {
    return <p>No events.</p>
  }

  return (
    <table>
      <thead>
        <tr>
          {AD_EVENT_COLUMNS.map((header) => (
            <th key={header}>{header}</th>
          ))}
        </tr>
      </thead>
      <tbody>
        {uniqueTableRows(data).map((row, index) => (
          <tr key={index}>
            {AD_EVENT_COLUMNS.map((header) => (
              <td key={header}>
                {header === 'Created At'
                  ? formatUnixEpochToLocalTime(row[header])
                  : row[header]}
              </td>
            ))}
          </tr>
        ))}
      </tbody>
    </table>
  )
}

export function Events() {
  const actions = useAppActions()
  const adEvents = useAppState((state) => state.adEvents)

  return (
    <div className='content-card'>
      <h4>
        <span className='title'>Active events</span>
        <Button
          size='small'
          onClick={actions.loadAdsInternals}
        >
          Refresh
        </Button>
      </h4>

      <section>
        <AdEventTable data={adEvents} />
      </section>
    </div>
  )
}
