/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { ConversionUrlPattern } from '../lib/app_store'
import { renderCopyableText } from '../lib/copyable_text'
import {
  formatRelativeDuration,
  formatUnixEpochToLocalTime,
  uniqueTableRows,
} from '../lib/format_time'
import { useCopyToClipboard } from './copy_toast'
import { TabHeader } from './tab_header'
import { WildcardPattern } from './wildcard_pattern'

type ColumnKey = keyof ConversionUrlPattern

const CONVERSION_URL_PATTERN_COLUMNS: ColumnKey[] = [
  'Creative Set ID',
  'Ad Type',
  'URL Pattern',
  'Observation Window',
  'Expires At',
]

// Explicit widths for every column so `table-layout: fixed` doesn't squeeze
// unclassed columns into whatever sliver is left over, which visibly
// overflowed one column's content into the next on a narrow window; sums to
// 100%. "Creative Set ID" is already ellipsis-truncated with a copy tooltip,
// so it can afford to give up room to "URL Pattern"/"Ad Type", which are
// shown in full.
const COLUMN_WIDTH_CLASSES: Partial<Record<ColumnKey, string>> = {
  'Creative Set ID': 'narrow-value-column',
  'Ad Type': 'narrow-value-column',
  'URL Pattern': 'rule-column',
  'Observation Window': 'value-column',
  'Expires At': 'value-column',
}

const TRUNCATED_COLUMNS = new Set<ColumnKey>(['Creative Set ID'])

function widthClassName(header: ColumnKey) {
  return [
    TRUNCATED_COLUMNS.has(header) ? 'truncate-cell nowrap-cell' : '',
    COLUMN_WIDTH_CLASSES[header] ?? '',
  ].join(' ').trim()
}

function formatObservationWindow(days: number) {
  return `${days} day${days === 1 ? '' : 's'}`
}

function ConversionUrlPatternTable({ data }: { data: ConversionUrlPattern[] }) {
  const copy = useCopyToClipboard()

  return (
    <>
      <table>
        <thead>
          <tr>
            {CONVERSION_URL_PATTERN_COLUMNS.map((header) => (
              <th key={header} className={widthClassName(header)}>
                {header}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {uniqueTableRows(data).map((row, index) => (
            <tr key={index}>
              {CONVERSION_URL_PATTERN_COLUMNS.map((header) => (
                <td key={header} className={widthClassName(header)}>
                  {header === 'Expires At'
                    ? <>
                        {formatUnixEpochToLocalTime(row[header])}{' '}
                        <span className='diagnostic-muted'>
                          ({formatRelativeDuration(row[header])})
                        </span>
                      </>
                    : header === 'Observation Window'
                      ? formatObservationWindow(row[header])
                      : header === 'URL Pattern'
                        ? (
                          <WildcardPattern
                            pattern={row[header]}
                            chars={['*']}
                            onCopy={copy}
                          />
                        )
                        : header === 'Creative Set ID'
                          ? renderCopyableText(row[header], `${index}`, copy)
                          : row[header]}
                </td>
              ))}
            </tr>
          ))}
        </tbody>
      </table>
      {data.length === 0 && (
        <p>No conversion URL patterns are currently being matched.</p>
      )}
    </>
  )
}

export function Conversions() {
  const actions = useAppActions()
  const conversionUrlPatterns = useAppState(
    (state) => state.conversionUrlPatterns,
  )

  React.useEffect(() => {
    actions.loadAdsInternals()
  }, [])

  return (
    <div className='card-group'>
      <TabHeader
        title={
          <>
            Active conversion URL patterns{' '}
            <span className='diagnostic-muted'>
              ({conversionUrlPatterns.length})
            </span>
          </>
        }
        description='URL patterns currently being matched against site
          visits to record a conversion.'
        onRefresh={actions.loadAdsInternals}
      />

      <div className='content-card'>
        <section>
          <ConversionUrlPatternTable data={conversionUrlPatterns} />
        </section>
      </div>
    </div>
  )
}
