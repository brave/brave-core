// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import scrollerStyles from './scroller.module.scss'

/** Delay before hiding the scrollbar after scrolling stops (macOS-like). */
const SCROLLBAR_HIDE_DELAY_MS = 1000

/**
 * Shows the scrollbar while the user scrolls, then hides it after a short delay.
 * Pair with `scroller.module.scss` on the scroll container element.
 */
export function useAutoHideScrollbar(
  scrollElementRef: React.RefObject<HTMLElement | null>,
) {
  React.useEffect(() => {
    const element = scrollElementRef.current
    if (!element) {
      return
    }

    let hideTimeoutId: ReturnType<typeof setTimeout> | undefined

    const showScrollbar = () => {
      element.classList.add(scrollerStyles.scrollbarVisible)
      if (hideTimeoutId !== undefined) {
        clearTimeout(hideTimeoutId)
      }
      hideTimeoutId = setTimeout(() => {
        element.classList.remove(scrollerStyles.scrollbarVisible)
        hideTimeoutId = undefined
      }, SCROLLBAR_HIDE_DELAY_MS)
    }

    element.addEventListener('scroll', showScrollbar, { passive: true })

    return () => {
      element.removeEventListener('scroll', showScrollbar)
      if (hideTimeoutId !== undefined) {
        clearTimeout(hideTimeoutId)
      }
      element.classList.remove(scrollerStyles.scrollbarVisible)
    }
  }, [scrollElementRef])
}
