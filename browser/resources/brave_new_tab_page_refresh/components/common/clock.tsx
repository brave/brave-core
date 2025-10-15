/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ClockFormat } from '../../state/new_tab_state'
import { useNewTabState } from '../../context/new_tab_context'

// Helper function for formatting the clock
export function formatClock(date: Date, clockFormat: ClockFormat) {
  const formatter = new Intl.DateTimeFormat(undefined, {
    hour: 'numeric',
    minute: 'numeric',
    hourCycle:
      clockFormat === ClockFormat.k12 ? 'h12' :
        clockFormat === ClockFormat.k24 ? 'h23' :
          undefined,
  })

  let formatted = formatter
    .formatToParts(date)
    .filter((item) => item.type !== 'dayPeriod')
    .map((item) => item.value)
    .join('')

  // Defensive fallback: replace 24:xx with 00:xx
  if (clockFormat === ClockFormat.k24 && /^24:/.test(formatted)) {
    formatted = formatted.replace(/^24:/, '00:')
  }

  return formatted
}

export function Clock() {
  const showClock = useNewTabState((s) => s.showClock)
  const clockFormat = useNewTabState((s) => s.clockFormat)

  const ref = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    function update() {
      if (ref.current) {
        ref.current.innerText = formatClock(new Date(), clockFormat)
      }
    }

    update()
    const timer = setInterval(update, 2000)
    return () => clearInterval(timer)
  }, [showClock, clockFormat])

  if (!showClock) {
    return null
  }

  return <div ref={ref} />
}
