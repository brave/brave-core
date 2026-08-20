/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useAppState, useAppActions } from '../lib/app_context'
import { ConversionUrlPattern } from '../lib/app_store'
import { formatUnixEpochToLocalTime, uniqueTableRows } from '../lib/format_time'

const CONVERSION_URL_PATTERN_COLUMNS: Array<keyof ConversionUrlPattern> = [
  'URL Pattern',
  'Expires At',
]

function ConversionUrlPatternTable({ data }: { data: ConversionUrlPattern[] }) {
  if (data.length === 0) {
    return <p>No conversion URL patterns are currently being matched.</p>
  }

  return (
    <table>
      <thead>
        <tr>
          {CONVERSION_URL_PATTERN_COLUMNS.map((header) => (
            <th key={header}>{header}</th>
          ))}
        </tr>
      </thead>
      <tbody>
        {uniqueTableRows(data).map((row, index) => (
          <tr key={index}>
            {CONVERSION_URL_PATTERN_COLUMNS.map((header) => (
              <td key={header}>
                {header === 'Expires At'
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

export function Conversions() {
  const actions = useAppActions()
  const conversionUrlPatterns = useAppState(
    (state) => state.conversionUrlPatterns,
  )

  return (
    <div className='content-card'>
      <h4>
        <span className='title'>Active conversion URL patterns</span>
        <Button
          size='small'
          onClick={actions.loadAdsInternals}
        >
          Refresh
        </Button>
      </h4>

      <section>
        <ConversionUrlPatternTable data={conversionUrlPatterns} />
      </section>
    </div>
  )
}
