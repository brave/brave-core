/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The native file picker (and programmatic <a> download clicks) can cause
// Leo's <dialog> to fire `cancel` / outside-click handlers and close the Brave
// News customize dialog. Callers wrap those interactions with
// `runWithCustomizeDialogCloseSuppressed` so Modal can ignore the spurious
// close and remount if needed.

let suppressCount = 0

export function isCustomizeDialogCloseSuppressed() {
  return suppressCount > 0
}

interface SuppressOptions {
  // When true, keep suppression until the window is focused again (after the
  // native file picker closes). When false, clear after a short delay.
  waitForWindowFocus?: boolean
}

export function runWithCustomizeDialogCloseSuppressed<T>(
  action: () => T,
  options?: SuppressOptions,
): T {
  suppressCount++
  try {
    return action()
  } finally {
    let ended = false
    const end = () => {
      if (ended) {
        return
      }
      ended = true
      window.removeEventListener('focus', end)
      // Let Leo finish handling any cancel/close dispatched as focus returns.
      window.setTimeout(() => {
        suppressCount = Math.max(0, suppressCount - 1)
      }, 100)
    }

    if (options?.waitForWindowFocus) {
      window.addEventListener('focus', end)
      // Safety net if focus never fires.
      window.setTimeout(end, 60_000)
    } else {
      window.setTimeout(end, 100)
    }
  }
}
