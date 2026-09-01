/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { DateRangeFilterState, DateRangePreset } from '../lib/app_store'

const DATE_RANGE_PRESET_LABELS: Record<DateRangePreset, string> = {
  all: 'All',
  today: 'Today',
  hour: 'Last hour',
  day: 'Last 24 hours',
  week: 'Last week',
  month: 'Last month',
  custom: 'Custom',
}

const DATE_RANGE_PRESET_SECONDS_AGO: Partial<Record<DateRangePreset, number>> = {
  hour: 3600,
  day: 86400,
  week: 7 * 86400,
  month: 30 * 86400,
}

// Local midnight, as opposed to "day" which is a rolling 24 hours back from
// right now.
function startOfToday() {
  const date = new Date()
  date.setHours(0, 0, 0, 0)
  return date.getTime() / 1000
}

// Dates come from `<input type="date">`, which reports/accepts the local
// day as "YYYY-MM-DD" with no time component.
function parseDateInput(value: string, endOfDay: boolean) {
  if (!value) {
    return null
  }
  const date = new Date(`${value}T${endOfDay ? '23:59:59.999' : '00:00:00'}`)
  return date.getTime() / 1000
}

export function computeDateRange(
  preset: DateRangePreset,
  fromDate: string,
  toDate: string,
): { fromSeconds: number | null, toSeconds: number | null } {
  if (preset === 'today') {
    return { fromSeconds: startOfToday(), toSeconds: null }
  }
  const secondsAgo = DATE_RANGE_PRESET_SECONDS_AGO[preset]
  if (secondsAgo !== undefined) {
    return { fromSeconds: Date.now() / 1000 - secondsAgo, toSeconds: null }
  }
  if (preset === 'custom') {
    return {
      fromSeconds: parseDateInput(fromDate, /*endOfDay=*/false),
      toSeconds: parseDateInput(toDate, /*endOfDay=*/true),
    }
  }
  return { fromSeconds: null, toSeconds: null }
}

export function DateRangeFilter({
  filter,
  setFilter,
  defaultPreset,
}: {
  filter: DateRangeFilterState
  setFilter: (filter: DateRangeFilterState) => void
  defaultPreset: DateRangePreset
}) {
  const { preset, fromDate, toDate } = filter

  return (
    <h4>
      <select
        value={preset}
        onChange={(event) => {
          setFilter({ ...filter, preset: event.target.value as DateRangePreset })
        }}
      >
        {Object.entries(DATE_RANGE_PRESET_LABELS).map(([value, label]) => (
          <option key={value} value={value}>{label}</option>
        ))}
      </select>
      {preset === 'custom' && (
        <>
          <span>From</span>
          <input
            type='date'
            value={fromDate}
            onChange={(event) => setFilter({ ...filter, fromDate: event.target.value })}
          />
          <span>To</span>
          <input
            type='date'
            value={toDate}
            onChange={(event) => setFilter({ ...filter, toDate: event.target.value })}
          />
        </>
      )}
      {preset !== defaultPreset && (
        <Button
          size='small'
          kind='plain-faint'
          onClick={() => setFilter({ preset: defaultPreset, fromDate: '', toDate: '' })}
        >
          Clear filter
        </Button>
      )}
    </h4>
  )
}
