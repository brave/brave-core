/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useAppState, useAppActions } from '../lib/app_context'
import { AdEvent } from '../lib/app_store'
import { RelatedCopyText, renderCopyableText } from '../lib/copyable_text'
import {
  formatUnixEpochToLocalDate,
  formatUnixEpochToLocalTime,
  formatUnixEpochToLocalTimeOnly,
  uniqueTableRows,
} from '../lib/format_time'
import { useCopyToClipboard } from './copy_toast'
import { computeDateRange, DateRangeFilter } from './date_range_filter'
import { TabHeader } from './tab_header'

// "Created At" is dropped in favor of one content-card per date (see `Events`
// below); repeating the same date on every row for events created close
// together just adds noise.
const AD_EVENT_COLUMNS: Array<keyof AdEvent> = [
  'Placement ID',
  'Creative Instance ID',
  'Target URL',
  'Ad Type',
  'Event Type',
]

const TRUNCATED_COLUMNS = new Set<keyof AdEvent>([
  'Placement ID',
  'Creative Instance ID',
  'Target URL',
])

// Explicit widths for every column (including the hardcoded "Time" column
// below) so `table-layout: fixed` doesn't squeeze the short Ad Type/Event
// Type/Time columns into whatever sliver is left over; sums to 100%. The ID
// columns are already ellipsis-truncated with a copy tooltip, so they can
// afford to give up room to Ad Type/Event Type, which are shown in full and
// need enough width for values like "ad_notification".
const COLUMN_WIDTH_CLASSES: Partial<Record<keyof AdEvent, string>> = {
  'Placement ID': 'narrow-value-column',
  'Creative Instance ID': 'narrow-value-column',
  'Target URL': 'rule-column',
  'Ad Type': 'narrow-value-column',
  'Event Type': 'narrow-value-column',
}

function buildPlacementEventsMap(rows: AdEvent[]) {
  const map = new Map<string, AdEvent[]>()
  for (const row of rows) {
    const placementId = row['Placement ID']
    const events = map.get(placementId)
    if (events) {
      events.push(row)
    } else {
      map.set(placementId, [row])
    }
  }
  return map
}

// Always returned (even when `current` is the only event for its placement
// ID), so the Cmd/Ctrl+click hint is consistent across every Placement ID
// cell rather than only appearing once a second event has been recorded.
function formatOtherPlacementEvents(
  events: AdEvent[],
  current: AdEvent,
): RelatedCopyText {
  const others = events.filter((event) => event !== current)
  const sorted = [current, ...others].sort(
    (a, b) => b['Created At'] - a['Created At'])
  return {
    hint: 'Cmd/Ctrl+click to copy other events with this placement ID',
    text: [
      `Placement ID: ${current['Placement ID']}`,
      ...sorted.map((event) =>
        `${formatUnixEpochToLocalTime(event['Created At'])} | ` +
          `${event['Ad Type']} | ${event['Event Type']}`),
    ].join('\n'),
  }
}

function downloadAdEvents(rows: AdEvent[]) {
  const lines = rows.map((row) => [
    formatUnixEpochToLocalTime(row['Created At']),
    `Placement ID: ${row['Placement ID']}`,
    `Creative Instance ID: ${row['Creative Instance ID']}`,
    `Ad Type: ${row['Ad Type']}`,
    `Event Type: ${row['Event Type']}`,
    `Target URL: ${row['Target URL']}`,
  ].join(' | '))
  const content = lines.join('\n')
  const filename = 'brave_ads_internals_events.txt'
  const element = document.createElement('a')
  element.setAttribute(
    'href',
    'data:text/plain;charset=utf-8,' + encodeURIComponent(content),
  )
  element.setAttribute('download', filename)
  element.style.display = 'none'
  document.body.appendChild(element)
  element.click()
  document.body.removeChild(element)
}

function AdEventTable({ data, allRows }: { data: AdEvent[], allRows: AdEvent[] }) {
  const copy = useCopyToClipboard()
  const placementEventsMap = React.useMemo(
    () => buildPlacementEventsMap(allRows),
    [allRows],
  )

  return (
    <table>
      <thead>
        <tr>
          {AD_EVENT_COLUMNS.map((header) => (
            <th
              key={header}
              className={[
                TRUNCATED_COLUMNS.has(header) ? 'truncate-cell' : '',
                COLUMN_WIDTH_CLASSES[header] ?? '',
              ].join(' ').trim()}
            >
              {header}
            </th>
          ))}
          <th className='status-column'>Time</th>
        </tr>
      </thead>
      <tbody>
        {data.map((row, index) => (
          <tr key={index}>
            {AD_EVENT_COLUMNS.map((header) => {
              const className = [
                TRUNCATED_COLUMNS.has(header) ? 'truncate-cell' : '',
                COLUMN_WIDTH_CLASSES[header] ?? '',
              ].join(' ').trim()
              if (!TRUNCATED_COLUMNS.has(header)) {
                return <td key={header} className={className}>{row[header]}</td>
              }
              return (
                <td key={header} className={className}>
                  {renderCopyableText(row[header] as string, `${index}-${header}`, copy,
                    header === 'Placement ID'
                      ? {
                          getRelated: (value) => {
                            const events = placementEventsMap.get(value)
                            return events
                              ? formatOtherPlacementEvents(events, row)
                              : undefined
                          },
                        }
                      : undefined)}
                </td>
              )
            })}
            <td className='status-column'>
              <span className='monospace-value'>
                {formatUnixEpochToLocalTimeOnly(row['Created At'])}
              </span>
            </td>
          </tr>
        ))}
      </tbody>
    </table>
  )
}

export function Events() {
  const actions = useAppActions()
  const adEvents = useAppState((state) => state.adEvents)
  const dateRangeFilter = useAppState((state) => state.eventsDateRangeFilter)
  const { preset, fromDate, toDate } = dateRangeFilter

  React.useEffect(() => {
    actions.loadAdsInternals()
  }, [])

  const { fromSeconds, toSeconds } = computeDateRange(preset, fromDate, toDate)

  // Consecutive rows created on the same day are grouped under one
  // content-card per date, so the date isn't repeated on every row. Sorted
  // most recent first, since the backend returns them oldest first.
  const { rows, groups } = React.useMemo(() => {
    const filteredRows = uniqueTableRows(adEvents)
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
    const groupedRows: Array<{ date: string, rows: AdEvent[] }> = []
    for (const row of filteredRows) {
      const date = formatUnixEpochToLocalDate(row['Created At'])
      const lastGroup = groupedRows.at(-1)
      if (lastGroup?.date === date) {
        lastGroup.rows.push(row)
      } else {
        groupedRows.push({ date, rows: [row] })
      }
    }
    return { rows: filteredRows, groups: groupedRows }
  }, [adEvents, fromSeconds, toSeconds])

  return (
    <div className='card-group'>
      <TabHeader
        title={
          <>
            Ad events{' '}
            <span className='diagnostic-muted'>({rows.length})</span>
          </>
        }
        description='View, click, landed, conversion, and reaction events
          recorded across all ad formats, grouped by date.'
        onRefresh={actions.loadAdsInternals}
        headerActions={groups.length > 0 && (
          <span className='fixed-flex-item'>
            <Button
              size='small'
              onClick={() => downloadAdEvents(rows)}
            >
              Download
            </Button>
          </span>
        )}
      >
        <DateRangeFilter
          filter={dateRangeFilter}
          setFilter={actions.setEventsDateRangeFilter}
          defaultPreset='day'
        />
        {groups.length === 0 && (
          <p>No ad events match the selected date range.</p>
        )}
      </TabHeader>
      {groups.map((group) => (
        <div
          key={group.date}
          className='content-card'
        >
          <h4>
            <span className='title'>
              {group.date}{' '}
              <span className='diagnostic-muted'>({group.rows.length})</span>
            </span>
          </h4>

          <section>
            <AdEventTable data={group.rows} allRows={rows} />
          </section>
        </div>
      ))}
    </div>
  )
}
