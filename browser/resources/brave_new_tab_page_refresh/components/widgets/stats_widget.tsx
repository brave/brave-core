/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useNewTabState } from '../../context/new_tab_context'
import { getString } from '../../lib/strings'

import { style } from './stats_widget.style'

const adsBlockedFormatter = new Intl.NumberFormat(undefined, {
  maximumFractionDigits: 0,
  useGrouping: true
})

export function StatsWidget() {
  const showStats = useNewTabState((s) => s.showShieldsStats)
  const stats = useNewTabState((s) => s.shieldsStats)

  function renderUnits(parts: Intl.NumberFormatPart[]) {
    return parts.map(({ type, value }) => {
      if (type === 'unit') {
        return <span key='unit' className='units'>{value}</span>
      }
      return value
    })
  }

  if (!showStats) {
    return null
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='title'>
        {getString('statsTitle')}
      </div>
      <div className='data'>
        <div>
          <div className='ads-blocked'>
            <div className='value'>
              {stats && adsBlockedFormatter.format(stats.adsBlocked)}
            </div>
            {getString('statsAdsBlockedText')}
          </div>
          <div className='bandwidth-saved'>
            <div className='value'>
              {stats && renderUnits(formatBandwidth(stats.bandwidthSavedBytes))}
            </div>
            {getString('statsBandwidthSavedText')}
          </div>
          <div className='time-saved'>
            <div className='value'>
              {stats &&
                renderUnits(formatTimeSaved(getTimeSaved(stats.adsBlocked)))}
            </div>
            {getString('statsTimeSavedText')}
          </div>
        </div>
      </div>
    </div>
  )
}

function getTimeSaved(adsBlocked: number) {
  return adsBlocked * 50
}

function formatTimeInterval(
  value: number,
  unit: 'day' | 'hour' | 'minute' | 'second',
  maximumFractionDigits: number = 0
) {
  return new Intl.NumberFormat(undefined, {
    style: 'unit',
    unit,
    unitDisplay: 'long',
    maximumFractionDigits,
    roundingMode: 'ceil'
  }).formatToParts(value)
}

function formatTimeSaved(ms: number) {
  const seconds = ms / 1000
  const minutes = seconds / 60
  const hours = minutes / 60
  const days = hours / 24

  if (days >= 1) {
    return formatTimeInterval(days, 'day', 2)
  }
  if (hours >= 1) {
    return formatTimeInterval(hours, 'hour', 1)
  }
  if (minutes >= 1) {
    return formatTimeInterval(minutes, 'minute')
  }
  if (seconds >= 1) {
    return formatTimeInterval(seconds, 'second')
  }
  return formatTimeInterval(0, 'second')
}

function formatMemoryValue(
  value: number,
  unit: 'kilobyte' | 'megabyte' | 'gigabyte',
  maximumFractionDigits: number = 0
) {
  return new Intl.NumberFormat(undefined, {
    style: 'unit',
    unit,
    unitDisplay: 'short',
    maximumFractionDigits,
    roundingMode: 'ceil'
  }).formatToParts(value)
}

function formatBandwidth(bytes: number) {
  const kb = bytes / 1024
  const mb = kb / 1024
  const gb = mb / 1024

  if (gb >= 1) {
    return formatMemoryValue(gb, 'gigabyte', 2)
  }
  if (mb >= 1) {
    return formatMemoryValue(mb, 'megabyte', 1)
  }
  if (kb >= 1) {
    return formatMemoryValue(kb, 'kilobyte')
  }
  return formatMemoryValue(kb, 'kilobyte')
}
