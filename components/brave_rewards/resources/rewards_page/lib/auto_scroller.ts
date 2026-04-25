/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Manages programmatic autoscrolling during page load. Returns a function that
// executes a scrolling callback only if the user hasn't manually scrolled and
// the time window hasn't expired. Programmatic scrolls are flagged so they
// don't trip user-scroll detection.
export function useAutoScroller(windowMs = 3000) {
  const expired = React.useRef(false)
  const userScrolled = React.useRef(false)
  const programmatic = React.useRef(false)

  React.useEffect(() => {
    const onScroll = () => {
      if (!programmatic.current) {
        userScrolled.current = true
      }
    }
    document.addEventListener('scroll', onScroll, true)
    const timeout = setTimeout(() => {
      expired.current = true
    }, windowMs)
    return () => {
      document.removeEventListener('scroll', onScroll, true)
      clearTimeout(timeout)
    }
  }, [])

  return React.useCallback((fn: () => void) => {
    if (userScrolled.current || expired.current) {
      return
    }
    programmatic.current = true
    fn()
    requestAnimationFrame(() => {
      programmatic.current = false
    })
  }, [])
}
