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
import { computeDateRange, DateRangeFilter } from './date_range_filter'
import { AD_EVENT_COLUMNS, AdEventTable } from './events'
import { TabHeader } from './tab_header'
import { WildcardPattern } from './wildcard_pattern'

// `ToString(mojom::ConfirmationType::kConversion)`; matches the "Event Type"
// value `BuildAdsInternals` bakes into `adEvents` for a conversion event.
const CONVERSION_EVENT_TYPE = 'conversion'

// Every row here is already a conversion, so "Event Type" would just repeat
// the section title rather than add information.
const CONVERSION_EVENT_COLUMNS = AD_EVENT_COLUMNS.filter(
  (column) => column !== 'Event Type',
)

type ColumnKey = keyof ConversionUrlPattern

const CONVERSION_URL_PATTERN_COLUMNS: ColumnKey[] = [
  'Campaign ID',
  'Ad Type',
  'Checkout URL Pattern',
  'Thank You URL Pattern',
  'Observation Window',
  'Expires At',
]

// Explicit widths for every column so `table-layout: fixed` doesn't squeeze
// unclassed columns into whatever sliver is left over, which visibly
// overflowed one column's content into the next on a narrow window; sums to
// 100%. "Campaign ID" is already ellipsis-truncated with a copy tooltip,
// so it can afford to give up room to the URL pattern columns/"Ad Type",
// which are shown in full.
const COLUMN_WIDTH_CLASSES: Partial<Record<ColumnKey, string>> = {
  'Campaign ID': 'narrow-value-column',
  'Ad Type': 'value-column',
  'Checkout URL Pattern': 'rule-column',
  'Thank You URL Pattern': 'rule-column',
  'Observation Window': 'narrow-value-column',
  'Expires At': 'value-column',
}

const TRUNCATED_COLUMNS = new Set<ColumnKey>(['Campaign ID'])

// Shown in full rather than truncated, but still kept to one line since the
// widened column already gives it enough room.
const NOWRAP_COLUMNS = new Set<ColumnKey>(['Ad Type'])

function widthClassName(header: ColumnKey) {
  return [
    TRUNCATED_COLUMNS.has(header) ? 'truncate-cell nowrap-cell' : '',
    NOWRAP_COLUMNS.has(header) ? 'nowrap-cell' : '',
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
                      : header === 'Checkout URL Pattern' ||
                          header === 'Thank You URL Pattern'
                        ? (
                          row[header]
                            ? (
                              <WildcardPattern
                                pattern={row[header]}
                                chars={['*']}
                                onCopy={copy}
                              />
                            )
                            : <span className='diagnostic-muted'>Not set</span>
                        )
                        : header === 'Campaign ID'
                          ? renderCopyableText(row[header], `${index}`, copy)
                          : row[header]}
                </td>
              ))}
            </tr>
          ))}
        </tbody>
      </table>
      {data.length === 0 && (
        <p>No conversion URL patterns are being matched.</p>
      )}
    </>
  )
}

// Plain sub-heading, not a second `TabHeader`, since the page-level
// `TabHeader` in `Conversions` below already owns the refresh action.
function ConversionEvents() {
  const adEvents = useAppState((state) => state.adEvents)
  const dateRangeFilter = useAppState(
    (state) => state.conversionsDateRangeFilter,
  )
  const actions = useAppActions()
  const { preset, fromDate, toDate } = dateRangeFilter
  const { fromSeconds, toSeconds } = computeDateRange(preset, fromDate, toDate)

  const rows = React.useMemo(() => {
    return uniqueTableRows(adEvents)
      .filter((row) => row['Event Type'] === CONVERSION_EVENT_TYPE)
      .filter((row) => {
        const createdAt = row['Created At']
        if (fromSeconds !== null && createdAt < fromSeconds) {
          return false
        }
        if (toSeconds !== null && createdAt > toSeconds) {
          return false
        }
        return true
      })
      .sort((a, b) => b['Created At'] - a['Created At'])
  }, [adEvents, fromSeconds, toSeconds])

  return (
    <div className='content-card'>
      <section>
        <h4>
          <span className='title'>
            Conversion events{' '}
            <span className='diagnostic-muted'>({rows.length})</span>
          </span>
        </h4>
        <p>
          Conversion events across all ad formats within the selected time
          range.
        </p>
        <DateRangeFilter
          filter={dateRangeFilter}
          setFilter={actions.setConversionsDateRangeFilter}
          defaultPreset='day'
        />
        <AdEventTable
          data={rows}
          allRows={adEvents}
          columns={CONVERSION_EVENT_COLUMNS}
        />
        {rows.length === 0 && (
          <p>No conversions match the selected date range.</p>
        )}
      </section>
    </div>
  )
}

export function Conversions() {
  const actions = useAppActions()
  const conversionUrlPatterns = useAppState(
    (state) => state.conversionUrlPatterns,
  )
  const adsInternalsVerboseModeEnabled = useAppState(
    (state) => state.adsInternalsVerboseModeEnabled,
  )

  React.useEffect(() => {
    actions.loadAdsInternals()
  }, [])

  return (
    <>
      <TabHeader
        title='Conversions'
        description='Overview of active conversion URL patterns.'
        onRefresh={actions.loadAdsInternals}
      />

      <div className='card-group'>
        <div className='content-card'>
          <section>
            <h4>
              <span className='title'>
                Active conversion URL patterns{' '}
                <span className='diagnostic-muted'>
                  ({conversionUrlPatterns.length})
                </span>
              </span>
            </h4>
            <p>
              URL patterns currently monitored to identify checkout and thank
              you pages for conversion tracking.
            </p>
            <ConversionUrlPatternTable data={conversionUrlPatterns} />
          </section>
        </div>

        {adsInternalsVerboseModeEnabled && <ConversionEvents />}
      </div>
    </>
  )
}
