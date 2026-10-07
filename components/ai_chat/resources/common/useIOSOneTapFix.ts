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

function isInsideLeoDialog(path: EventTarget[]): boolean {
  return path.some(
    (n) => n instanceof Element && n.tagName.toLowerCase() === 'leo-dialog',
  )
}

/**
 * Programmatic HTMLElement.click() synthesizes a MouseEvent at (0, 0).
 * Leo Dialog treats that as an outside click and closes. When the tap is
 * inside a dialog, dispatch with the real touch coordinates instead.
 * Outside dialogs, prefer .click() so default actions (e.g. links) still run.
 */
function activateElement(
  el: HTMLElement,
  path: EventTarget[],
  clientX: number,
  clientY: number,
): void {
  if (isInsideLeoDialog(path)) {
    el.dispatchEvent(
      new MouseEvent('click', {
        bubbles: true,
        cancelable: true,
        view: window,
        clientX,
        clientY,
      }),
    )
    return
  }
  el.click()
}

/**
 * Leo Dropdown opens via a button inside its shadow root.
 * Clicking the host doesn't trigger the toggle; we must click
 * that inner button so one tap opens the dropdown.
 */
function clickLeoDropdownTrigger(
  host: Element,
  path: EventTarget[],
  clientX: number,
  clientY: number,
): boolean {
  const root = (host as HTMLElement).shadowRoot
  if (!root) return false
  const button =
    root.querySelector('button.click-target')
    ?? root.querySelector('.leo-dropdown button')
  if (button instanceof HTMLElement) {
    activateElement(button, path, clientX, clientY)
    return true
  }
  return false
}

/** Max movement (px) from touchstart to touchend to count as a tap (not a drag). */
const TAP_MOVE_THRESHOLD = 10

/** Returns true if the tap was handled; else caller should dispatch click. */
function handleTapPath(
  path: EventTarget[],
  e: TouchEvent,
  clientX: number,
  clientY: number,
): boolean {
  for (const node of path) {
    if (node instanceof Element) {
      const el = node as HTMLElement
      const tag = el.tagName?.toLowerCase() ?? ''
      // ContentEditable (e.g. chat input): focus on first tap so iOS
      // doesn't require double-tap to focus.
      if (el.isContentEditable) {
        el.focus()
        e.preventDefault()
        return true
      }
      // Dropdown: tap may land on host or content; open via inner button
      if (
        tag === 'leo-dropdown'
        && clickLeoDropdownTrigger(node, path, clientX, clientY)
      ) {
        e.preventDefault()
        return true
      }
      if (INTERACTIVE_TAGS.has(tag)) {
        e.preventDefault()
        activateElement(el, path, clientX, clientY)
        return true
      }
      // onTapElsewhere should not be called for Leo Inputs or Textareas
      // defaulting to true so the input or textarea is focused on first tap.
      if (
        tag === 'leo-input'
        || tag === 'leo-textarea'
        || tag === 'leo-dialog'
      ) {
        return true
      }
    }
  }
  return false
}

export interface UseIOSOneTapFixOptions {
  /**
   * When the user taps or clicks on non-interactive content ("tap elsewhere"),
   * this is called so the embedder can react (e.g. iframe notifying parent to
   * close menus via mojom). Called in addition to dispatching target.click().
   */
  onTapElsewhere?: () => void
}

/**
 * On iOS, some elements don't trigger click events consistently:
 * first tap focuses, second activates. This hook adds document-level touch
 * listeners: touchstart records position so touchend only triggers click when
 * the touch didn't move (avoids selecting a menu item after dragging in a menu).
 * Optional onTapElsewhere (e.g. iframe) notifies embedder to close menus.
 */
export function useIOSOneTapFix(options?: UseIOSOneTapFixOptions): void {
  const onTapElsewhere = options?.onTapElsewhere

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

      const { clientX, clientY } = endTouch
      const path = e.composedPath()

      if (handleTapPath(path, e, clientX, clientY)) return

      // Tap elsewhere (e.g. backdrop). Dispatch a click so
      // document-level listeners run—e.g. Leo ButtonMenu’s clickOutside closes
      // the menu on first tap instead of requiring double-tap on iOS.
      const target = e.target as Node
      if (target instanceof HTMLElement) {
        e.preventDefault()
        activateElement(target, path, clientX, clientY)
      }
      onTapElsewhere?.()
    }

    document.addEventListener('touchstart', handleTouchStart, { passive: true })
    document.addEventListener('touchend', handleTouchEnd)
    return () => {
      document.removeEventListener('touchstart', handleTouchStart)
      document.removeEventListener('touchend', handleTouchEnd)
    }
  }, [onTapElsewhere])
}
