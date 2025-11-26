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

  const ref = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    const formatter = new Intl.DateTimeFormat(undefined, {
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

    function update() {
      if (ref.current) {
        ref.current.innerText = formatter
          .formatToParts(new Date())
          .filter((item) => item.type !== 'dayPeriod')
          .map((item) => item.value)
          .join('')
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
