// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

/** Tag names for interactive elements that on iOS may need
 * two taps (focus then activate). */
const INTERACTIVE_TAGS = new Set([
  'leo-button',
  'leo-menu-item',
  'leo-option',
  'leo-dropdown',
  'a',
  'button',
])

/**
 * Leo Dropdown opens via a button inside its shadow root.
 * Clicking the host doesn't trigger the toggle; we must click
 * that inner button so one tap opens the dropdown.
 */
function clickLeoDropdownTrigger(host: Element): boolean {
  const root = (host as HTMLElement).shadowRoot
  if (!root) return false
  const button =
    root.querySelector('button.click-target')
    ?? root.querySelector('.leo-dropdown button')
  if (button instanceof HTMLElement) {
    button.click()
    return true
  }
  return false
}

/** Max movement (px) from touchstart to touchend to count as a tap (not a drag). */
const TAP_MOVE_THRESHOLD = 10

/**
 * On iOS, some elements don't trigger click events consistently:
 * first tap focuses, second activates. This hook adds document-level touch
 * listeners: touchstart records position so touchend only triggers click when
 * the touch didn't move (avoids selecting a menu item after dragging in a menu).
 */
export function useIOSOneTapFix(): void {
  React.useEffect(() => {
    let touchStart: { x: number; y: number } | null = null

    const handleTouchStart = (e: TouchEvent) => {
      if (e.touches[0]) {
        touchStart = {
          x: e.touches[0].clientX,
          y: e.touches[0].clientY,
        }
      }
    }

    const handleTouchEnd = (e: TouchEvent) => {
      const start = touchStart
      touchStart = null
      const endTouch = e.changedTouches[0]
      if (!start || !endTouch) return
      const move = Math.hypot(
        endTouch.clientX - start.x,
        endTouch.clientY - start.y,
      )
      if (move > TAP_MOVE_THRESHOLD) return

      const path = e.composedPath()
      for (const node of path) {
        if (node instanceof Element) {
          const tag = node.tagName?.toLowerCase() ?? ''

          // Dropdown: tap may land on host or content; open via inner button
          if (tag === 'leo-dropdown' && clickLeoDropdownTrigger(node)) {
            e.preventDefault()
            return
          }
          if (INTERACTIVE_TAGS.has(tag)) {
            e.preventDefault()
            ;(node as HTMLElement).click()
            return
          }
        }
      }
    }

    document.addEventListener('touchstart', handleTouchStart, { passive: true })
    document.addEventListener('touchend', handleTouchEnd)
    return () => {
      document.removeEventListener('touchstart', handleTouchStart)
      document.removeEventListener('touchend', handleTouchEnd)
    }
  }, [])
}
