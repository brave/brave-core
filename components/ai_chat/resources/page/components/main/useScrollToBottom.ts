// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

/**
 * @param scrollElement The element which scrolls
 * @param scrollContent The content of the scroller
 */
export function useScrollToBottom(
  scrollElement: React.RefObject<HTMLDivElement>,
  scrollContent: React.RefObject<HTMLDivElement>,
) {
  const [hasScrollableContent, setHasScrollableContent] = React.useState(false)

  // Check if we can scroll every time the size of the scrollable content changes so
  // we can determine if we should show the scroll to bottom button.
  React.useEffect(() => {
    const element = scrollElement.current
    if (!element) return

    const checkScrollable = () => {
      const isScrollable = element.scrollHeight > element.clientHeight
      setHasScrollableContent(isScrollable)
    }

    checkScrollable()
    const timeoutId = setTimeout(checkScrollable, 100)

    // Unfortunately a resize observer is the easiest way to track whether we're scrollable
    // as Safari doesn't support @container scroll-state(...) yet (Feb 2026)
    // https://caniuse.com/wf-container-scroll-state-queries
    const resizeObserver = new ResizeObserver(checkScrollable)
    resizeObserver.observe(element)

    if (scrollContent.current) {
      resizeObserver.observe(scrollContent.current)
    }

    return () => {
      clearTimeout(timeoutId)
      resizeObserver.disconnect()
    }
  }, [])

  // As some elements are lazy loaded we continue to try and scroll down for a couple of seconds.
  const scrollToBottomContinuously = React.useCallback(
    async (animate: boolean = true) => {
      if (!scrollElement.current) return

      const element = scrollElement.current

      // Track the start time and whether we've interacted with the page.
      const startTime = Date.now()
      const duration = 1000
      let isCancelled = false

      const cancelScroll = () => {
        isCancelled = true
      }

      // We cancel the scroll if the user starts interacting with the page. We need to do
      // this as we try to continuously scroll to ensure we finish scrolling after
      // elements are lazy loaded.
      element.addEventListener('touchstart', cancelScroll, {
        once: true,
        passive: true,
      })
      document.addEventListener('wheel', cancelScroll)
      document.addEventListener('keydown', cancelScroll)

      // Scroll until we cancel, or the duration has expired.
      while (Date.now() - startTime < duration) {
        if (isCancelled) break

        element.scrollTo({
          top: element.scrollHeight,
          behavior: animate ? 'smooth' : 'instant',
        })
        await new Promise((resolve) => requestAnimationFrame(resolve))
      }

      // Remove the event listeners.
      element.removeEventListener('touchstart', cancelScroll)
      document.removeEventListener('wheel', cancelScroll)
      document.removeEventListener('keydown', cancelScroll)
    },
    [],
  )

  return {
    scrollToBottomContinuously,
    hasScrollableContent,
  }
}
