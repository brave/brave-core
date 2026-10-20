/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

function secondsUntil(time: number) {
  return Math.max(0, Math.ceil((time - Date.now()) / 1000))
}

// Returns the whole number of seconds left until `time`, updated every
// second, and never less than zero.
export function useCountdown(time: number) {
  const [seconds, setSeconds] = React.useState(() => secondsUntil(time))

  React.useEffect(() => {
    setSeconds(secondsUntil(time))
    const id = setInterval(() => setSeconds(secondsUntil(time)), 1000)
    return () => clearInterval(id)
  }, [time])

  return seconds
}
