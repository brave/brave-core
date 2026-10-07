/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Keeps a single line of text within the width of the element rendering it,
// shortening it from the middle. Shared by the elements that show an account
// email, which is user-supplied and arbitrarily long.
export class TextTruncator {
  private measure?: (text: string) => number
  private observedElement?: HTMLElement
  private resizeObserver?: ResizeObserver
  private text = ''

  constructor(private readonly onTruncated: (text: string) => void) {}

  observe(element: HTMLElement, text: string) {
    this.text = text

    if (this.observedElement !== element) {
      this.disconnect()
      this.observedElement = element

      const ctx = document.createElement('canvas').getContext('2d')!
      ctx.font = getComputedStyle(element).font
      this.measure = (text: string) => ctx.measureText(text).width

      this.resizeObserver = new ResizeObserver(() => this.truncate())
      this.resizeObserver.observe(element)
    }

    // Re-truncate on every call so a live text update is reflected even when
    // the layout width is unchanged and the `ResizeObserver` does not fire.
    this.truncate()
  }

  disconnect() {
    this.measure = undefined
    this.observedElement = undefined
    this.resizeObserver?.disconnect()
    this.resizeObserver = undefined
  }

  private truncate() {
    if (!this.measure || !this.observedElement || !this.text) return

    // Elements such as `leo-input` lay the text out in an inner `input`, which
    // is what bounds it.
    const availableWidth = (
      this.observedElement.shadowRoot?.querySelector('input')
      ?? this.observedElement
    ).clientWidth
    if (!availableWidth || this.measure(this.text) <= availableWidth) {
      this.onTruncated(this.text)
      return
    }

    // Use binary search (O(log n)) to find the maximum number of characters
    // that fit within available width. Characters are split evenly between
    // the start and end of the text with '…' in the middle.
    const chars = Array.from(this.text)
    const makeCandidate = (kept: number): string => {
      const prefixLen = Math.ceil(kept / 2)
      const suffixLen = Math.floor(kept / 2)
      return (
        chars.slice(0, prefixLen).join('')
        + '…'
        + chars.slice(chars.length - suffixLen).join('')
      )
    }

    let truncated = ''
    for (let low = 0, high = chars.length; low < high; ) {
      const middle = Math.ceil((low + high) / 2)
      const candidate = makeCandidate(middle)
      if (this.measure(candidate) <= availableWidth) {
        truncated = candidate
        low = middle
      } else {
        high = middle - 1
      }
    }

    this.onTruncated(truncated)
  }
}
