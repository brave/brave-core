// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { parseEntryFragment } from '../../../common/entry_fragment'

// How long the entry's layout is given to settle, as images and rendered
// markdown change the height of what is above it.
const SETTLE_MS = 1000
// How long to wait for the entry to be rendered.
const WAIT_FOR_ENTRY_MS = 10000

// The attribute that names the entries a turn shows, separated by spaces.
export const ENTRY_UUIDS_ATTRIBUTE = 'data-entry-uuids'
// Set on the turn that was scrolled to, until its highlight has played.
export const HIGHLIGHT_ATTRIBUTE = 'data-highlighted'

function findTurn(root: HTMLElement, entryUuid: string) {
  return [
    ...root.querySelectorAll<HTMLElement>(`[${ENTRY_UUIDS_ATTRIBUTE}]`),
  ].find((turn) =>
    turn.getAttribute(ENTRY_UUIDS_ATTRIBUTE)?.split(' ').includes(entryUuid),
  )
}

/**
 * Scrolls to, and highlights, the entry the frame's URL fragment names, once
 * it has been rendered. Using the same fragment again scrolls again.
 *
 * Returns the entry the frame was opened at, which is none when it wasn't, so
 * that the frame can leave its own scrolling to the bottom out.
 */
export function useScrollToEntry(
  scrollElement: React.RefObject<HTMLElement | null>,
  contentElement: React.RefObject<HTMLElement | null>,
) {
  const [hash, setHash] = React.useState(window.location.hash)
  const target = React.useMemo(() => parseEntryFragment(hash), [hash])

  React.useEffect(() => {
    const onHashChange = () => setHash(window.location.hash)
    window.addEventListener('hashchange', onHashChange)
    return () => window.removeEventListener('hashchange', onHashChange)
  }, [])

  React.useEffect(() => {
    const scroller = scrollElement.current
    const content = contentElement.current
    if (!target || !scroller || !content) {
      return
    }

    let turn: HTMLElement | undefined
    let settleTimer: ReturnType<typeof setInterval> | undefined
    let giveUpTimer: ReturnType<typeof setTimeout> | undefined
    let observer: MutationObserver | undefined
    let removeHighlightListener: (() => void) | undefined

    // A scroll the user makes leaves the entry where they put it.
    const stop = () => {
      clearInterval(settleTimer)
      clearTimeout(giveUpTimer)
      observer?.disconnect()
      scroller.removeEventListener('wheel', stop)
      scroller.removeEventListener('touchmove', stop)
      scroller.removeEventListener('keydown', stop)
    }
    scroller.addEventListener('wheel', stop, { passive: true })
    scroller.addEventListener('touchmove', stop, { passive: true })
    scroller.addEventListener('keydown', stop)

    const reveal = () => {
      turn?.scrollIntoView({ block: 'center', behavior: 'instant' })
    }

    const found = (foundTurn: HTMLElement) => {
      turn = foundTurn
      observer?.disconnect()
      clearTimeout(giveUpTimer)
      // The highlight plays again for each use of the fragment.
      turn.removeAttribute(HIGHLIGHT_ATTRIBUTE)
      void turn.offsetWidth
      turn.setAttribute(HIGHLIGHT_ATTRIBUTE, '')
      const clearHighlight = () => turn?.removeAttribute(HIGHLIGHT_ATTRIBUTE)
      turn.addEventListener('animationend', clearHighlight, { once: true })
      removeHighlightListener = () =>
        turn?.removeEventListener('animationend', clearHighlight)
      reveal()
      const settleUntil = Date.now() + SETTLE_MS
      settleTimer = setInterval(() => {
        if (Date.now() > settleUntil) {
          stop()
          return
        }
        reveal()
      }, 100)
    }

    const existing = findTurn(content, target.entryUuid)
    if (existing) {
      found(existing)
    } else {
      // The history is still being fetched.
      observer = new MutationObserver(() => {
        const rendered = findTurn(content, target.entryUuid)
        if (rendered) {
          found(rendered)
        }
      })
      observer.observe(content, { childList: true, subtree: true })
      giveUpTimer = setTimeout(stop, WAIT_FOR_ENTRY_MS)
    }

    return () => {
      stop()
      removeHighlightListener?.()
    }
  }, [target, scrollElement, contentElement])

  return target
}
