// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Wallet dropdown shell: positions the menu (StyledWrapper) under or over the
 * trigger and caps height with overflow when content would leave the visible
 * scroll/viewport area.
 *
 * Layout rules (high level):
 * 1. Find a vertical "clip" range — prefer the nearest scrollable ancestor
 *    (e.g. portfolio token list); otherwise the window height.
 * 2. Flip: if the full menu does not fit below the trigger but fits better
 *    above, anchor the menu upward (CSS bottom on the offset parent).
 * 3. Max height: compute how much vertical space remains inside the clip for
 *    the current anchor; if natural content height exceeds that, set max-height
 *    and let StyledWrapper scroll (overflow-y: auto).
 *
 * DOM expectation: the menu element must be the immediate next sibling after
 * the clickable trigger (previousElementSibling from the menu root), inside a
 * position:relative offset parent — same pattern as AssetMenuWrapper rows.
 */

import * as React from 'react'

// Styled Components
import { StyledWrapper } from './wellet-menus.style'

/** Padding from clip edges when measuring available space (px). */
const MENU_VIEWPORT_MARGIN = 8

/**
 * Walks up from `node` and returns the first element that can scroll on the
 * Y axis. Used so "fits on screen" uses the list (or panel) viewport, not only
 * the browser window.
 */
function findScrollParent(node: HTMLElement | null): HTMLElement | null {
  let el = node?.parentElement ?? null
  while (el && el !== document.documentElement) {
    const { overflowY } = globalThis.getComputedStyle(el)
    if (
      overflowY === 'auto'
      || overflowY === 'scroll'
      || overflowY === 'overlay'
    ) {
      return el
    }
    el = el.parentElement
  }
  return null
}

/**
 * @returns Top and bottom of the region where the menu must stay visible, in
 *   viewport coordinates.
 */
function getClipBounds(menu: HTMLElement) {
  const scrollParent = findScrollParent(menu)
  if (!scrollParent) {
    return { clipTop: 0, clipBottom: globalThis.innerHeight }
  }
  const pr = scrollParent.getBoundingClientRect()
  return { clipTop: pr.top, clipBottom: pr.bottom }
}

/**
 * Decides whether to open the menu above the trigger.
 * Uses `scrollHeight` (full content height) so the choice stays stable even
 * when a max-height is already applied from a previous layout pass.
 */
function shouldOpenMenuUpward(
  menu: HTMLElement,
  trigger: HTMLElement,
  clipTop: number,
  clipBottom: number,
) {
  const menuHeight = menu.scrollHeight
  if (menuHeight < 1) {
    return false
  }

  const triggerRect = trigger.getBoundingClientRect()
  const spaceBelow = clipBottom - triggerRect.bottom - MENU_VIEWPORT_MARGIN
  const spaceAbove = triggerRect.top - clipTop - MENU_VIEWPORT_MARGIN

  if (spaceBelow >= menuHeight) {
    return false
  }
  if (spaceAbove >= menuHeight) {
    return true
  }
  return spaceAbove > spaceBelow
}

/**
 * Computes flip + optional max-height for one frame.
 *
 * Max-height geometry uses the menu's `offsetParent` and CSS offsets
 * (`yPosition` below, `verticalGap` above) so we do not depend on the menu
 * having been painted with the final flip direction yet.
 *
 * @returns `maxHeightPx` only when content is taller than the allowed band;
 *   otherwise undefined so StyledWrapper grows to content (no scrollbar).
 */
function computeMenuLayout(
  menu: HTMLElement,
  trigger: HTMLElement,
  flipPlacement: boolean,
  yPosition: number,
  verticalGap: number,
): { openUpward: boolean; maxHeightPx: number | undefined } {
  const { clipTop, clipBottom } = getClipBounds(menu)
  const openUpward =
    flipPlacement && shouldOpenMenuUpward(menu, trigger, clipTop, clipBottom)

  const naturalH = menu.scrollHeight
  const parent = menu.offsetParent as HTMLElement | null

  let maxHBoundary: number
  if (parent) {
    const parentRect = parent.getBoundingClientRect()
    if (openUpward) {
      // bottom: calc(100% + gap) pins the menu bottom just above the offset parent top
      const menuBottom = parentRect.top - verticalGap
      maxHBoundary = menuBottom - clipTop - MENU_VIEWPORT_MARGIN
    } else {
      const menuTop = parentRect.top + yPosition
      maxHBoundary = clipBottom - menuTop - MENU_VIEWPORT_MARGIN
    }
  } else {
    // e.g. fixed positioning — fall back to the menu's current box
    const r = menu.getBoundingClientRect()
    if (openUpward) {
      maxHBoundary = r.bottom - clipTop - MENU_VIEWPORT_MARGIN
    } else {
      maxHBoundary = clipBottom - r.top - MENU_VIEWPORT_MARGIN
    }
  }

  if (!Number.isFinite(maxHBoundary)) {
    return { openUpward, maxHeightPx: undefined }
  }

  const floorMax = Math.floor(maxHBoundary)
  if (floorMax <= 0 || floorMax >= naturalH) {
    return { openUpward, maxHeightPx: undefined }
  }

  return { openUpward, maxHeightPx: floorMax }
}

export interface WalletMenuWrapperProps {
  children: React.ReactNode
  /** CSS `top` offset (px) from the offset parent when opening downward. */
  yPosition?: number
  /** Gap (px) between trigger row and menu when opening upward. */
  $verticalGap?: number
  right?: number
  left?: number
  padding?: string
  /**
   * When true (default), opens above the trigger if there is not enough space
   * below. Expects the menu root to follow the trigger as the previous sibling
   * in the DOM (same pattern as portfolio asset rows).
   */
  flipPlacement?: boolean
}

/**
 * Renders wallet menu content inside `StyledWrapper` with automatic vertical
 * flip and scroll when needed. Forwards ref to the wrapper DOM node.
 */
export const MenuWrapper = React.forwardRef<
  HTMLDivElement,
  WalletMenuWrapperProps
>(function MenuWrapper(props, forwardedRef) {
  const {
    children,
    flipPlacement = true,
    $verticalGap = 8,
    yPosition,
    right,
    left,
    padding,
  } = props

  const yOffset = yPosition ?? 35

  const [menuLayout, setMenuLayout] = React.useState<{
    openUpward: boolean
    maxHeightPx: number | undefined
  }>({ openUpward: false, maxHeightPx: undefined })

  const innerRef = React.useRef<HTMLDivElement | null>(null)

  const setRefs = React.useCallback(
    (node: HTMLDivElement | null) => {
      innerRef.current = node
      if (typeof forwardedRef === 'function') {
        forwardedRef(node)
      } else if (forwardedRef != null) {
        forwardedRef.current = node
      }
    },
    [forwardedRef],
  )

  const updatePlacement = React.useCallback(() => {
    const menu = innerRef.current
    const trigger = menu?.previousElementSibling
    if (!menu || !trigger || !(trigger instanceof HTMLElement)) {
      setMenuLayout({ openUpward: false, maxHeightPx: undefined })
      return
    }
    setMenuLayout(
      computeMenuLayout(menu, trigger, flipPlacement, yOffset, $verticalGap),
    )
  }, [flipPlacement, yOffset, $verticalGap])

  React.useLayoutEffect(() => {
    const menu = innerRef.current
    if (!menu) {
      return
    }

    updatePlacement()
    // Second pass after the browser applies flip/max-height from the first pass
    const rafId = requestAnimationFrame(updatePlacement)

    const scrollParent = findScrollParent(menu)
    scrollParent?.addEventListener('scroll', updatePlacement, {
      passive: true,
    })
    globalThis.addEventListener('scroll', updatePlacement, { passive: true })
    globalThis.addEventListener('resize', updatePlacement)

    const resizeObserver = new ResizeObserver(updatePlacement)
    resizeObserver.observe(menu)

    return () => {
      cancelAnimationFrame(rafId)
      scrollParent?.removeEventListener('scroll', updatePlacement)
      globalThis.removeEventListener('scroll', updatePlacement)
      globalThis.removeEventListener('resize', updatePlacement)
      resizeObserver.disconnect()
    }
  }, [updatePlacement])

  return (
    <StyledWrapper
      ref={setRefs}
      yPosition={yPosition}
      right={right}
      left={left}
      padding={padding}
      $openUpward={flipPlacement && menuLayout.openUpward}
      $verticalGap={$verticalGap}
      $maxHeightPx={menuLayout.maxHeightPx}
    >
      {children}
    </StyledWrapper>
  )
})
