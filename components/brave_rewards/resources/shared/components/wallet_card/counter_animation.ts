/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

function cubicBezier (p0: number, p1: number, p2: number, p3: number) {
  return (t: number) => (
    p0 * (1 - t) ** 3 +
    p1 * t * 3 * (1 - t) ** 2 +
    p2 * 3 * (1 - t) * t ** 2 +
    p3 * t ** 3
  )
}

const easeInOut = cubicBezier(0.42, 0, 0.58, 1.0)

// A React hook that will slide a number from zero to the specified target
// value. If the target value changes, it will slide from the old value to the
// new value.
export function useCounterAnimation (targetValue: number, duration: number) {
  duration = Math.max(1, duration)

  const [value, setValue] = React.useState(0)

  React.useEffect(() => {
    if (value === targetValue) {
      return
    }
    const start = Date.now()
    let handle = 0
    const onAnimationFrame = () => {
      const elapsed = Date.now() - start
      if (elapsed < duration) {
        setValue(easeInOut(elapsed / duration) * targetValue)
        handle = requestAnimationFrame(onAnimationFrame)
      } else {
        setValue(targetValue)
      }
    }
    handle = requestAnimationFrame(onAnimationFrame)
    return () => cancelAnimationFrame(handle)
  }, [targetValue])

  return value
}
