/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useNewTabPref } from '../../../hooks/usePref'

import { StyledClock, StyledTime } from './style'

// Tick once every two seconds.
const TICK_RATE = 2000
export function Clock () {
  const [now, setNow] = React.useState<Date>()
  const [clockFormat, setClockFormat] = useNewTabPref('clockFormat')
  const toggleClockFormat = () => {
    switch (clockFormat) {
      case '': return setClockFormat('12')
      case '12': return setClockFormat('24')
      case '24': return setClockFormat('')
    }
  }

  React.useEffect(() => {
    const interval = setInterval(() => setNow(new Date()), TICK_RATE)
    return () => clearInterval(interval)
  }, [])

  const formatter = React.useMemo(() => new Intl.DateTimeFormat(undefined, {
    hour: 'numeric',
    minute: 'numeric',
    hourCycle: clockFormat === '12'
      ? 'h12'
      : clockFormat === '24'
        ? 'h23'
        // If clock format is not set, let Intl decide (use the system pref).
        : undefined
  }), [clockFormat])

  // Don't render AM/PM
  const formattedTime = React.useMemo(() => formatter.formatToParts(now)
    .map(t => t.type === 'dayPeriod'
      ? null
      : t.value), [formatter, now])

  return <StyledClock onDoubleClick={toggleClockFormat}>
    <StyledTime>{formattedTime}</StyledTime>
  </StyledClock>
}
