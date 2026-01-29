/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ClockFormat } from '../../state/new_tab_state'
import { useNewTabState } from '../../context/new_tab_context'

export function Clock() {
  const showClock = useNewTabState((s) => s.showClock)
  const clockFormat = useNewTabState((s) => s.clockFormat)
  const [time, setTime] = React.useState(Date.now())

  const formatter = React.useMemo(() => {
    return new Intl.DateTimeFormat(undefined, {
      hour: 'numeric',
      minute: 'numeric',
      hourCycle:
        clockFormat === ClockFormat.k12
          ? 'h12'
          : // For 24-hour mode, use h23, which starts at 0:00 instead of 24:00.
            clockFormat === ClockFormat.k24
            ? 'h23'
            : undefined,
    })
  }, [clockFormat])

  React.useEffect(() => {
    if (!showClock) {
      return
    }
    const timer = setInterval(() => setTime(Date.now()), 2000)
    return () => clearInterval(timer)
  }, [showClock])

  if (!showClock) {
    return null
  }

  return (
    <>
      {formatter.formatToParts(time).map((item) => {
        if (item.type === 'dayPeriod') {
          return <span className='day-period'>{item.value}</span>
        }
        return item.value
      })}
    </>
  )
}
