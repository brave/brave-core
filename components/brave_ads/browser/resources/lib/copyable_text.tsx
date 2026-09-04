/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { formatIdFamily, useIdFamily } from './id_family'

// Chrome component updater IDs are 32 lowercase "a"-"p" characters (a base16
// alphabet mapped onto letters), distinct in shape from a UUIDv4.
const COMPONENT_ID_REGEX = /\b[a-p]{32}\b/
const UUIDV4_REGEX =
  /\b[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}\b/i

// Anchored, full-string check (the exported regex above only detects a UUID
// embedded anywhere in a larger string).
const UUIDV4_EXACT_REGEX =
  /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i

export function isValidUuidV4(value: string) {
  return UUIDV4_EXACT_REGEX.test(value)
}
const URL_REGEX = /\bhttps?:\/\/[^\s"]+/i

// Non-capturing so `matchAll` reports each match without extra capture-group
// entries.
const COPYABLE_TEXT_REGEX = new RegExp(
  `(?:${COMPONENT_ID_REGEX.source})|(?:${UUIDV4_REGEX.source})|` +
    `(?:${URL_REGEX.source})`,
  'gi',
)

// Repeating the full value in the tooltip only matters when the value itself
// is visually ellipsis-clipped; otherwise it's just noise since the value is
// already fully visible on-screen. `.copyable-text` gives the span its own
// ellipsis-clipping box (see app.style.ts) so `scrollWidth`/`clientWidth`
// measured here reflect this span's own truncation, whether the clipping is
// caused by this span filling a `truncate-cell` or by it merely running out
// of room within flowing text.
export function CopyableSpan({ value, className, onCopy }: {
  value: string
  className: string
  onCopy: (value: string) => void
}) {
  const ref = React.useRef<HTMLSpanElement>(null)
  const [isTruncated, setIsTruncated] = React.useState(false)
  const idFamily = useIdFamily(value)

  function checkTruncation() {
    const element = ref.current
    if (element) {
      setIsTruncated(element.scrollWidth > element.clientWidth)
    }
  }

  const hints = ['Click to copy']
  if (idFamily) {
    hints.push('Cmd/Ctrl+click to copy related IDs')
  }
  const hintText = `(${hints.join('; ')})`

  return (
    <span
      ref={ref}
      className={className}
      title={isTruncated ? `${value} ${hintText}` : hintText}
      onMouseEnter={checkTruncation}
      onClick={(event) => {
        if (idFamily && (event.metaKey || event.ctrlKey)) {
          onCopy(formatIdFamily(idFamily))
        } else {
          onCopy(value)
        }
      }}
    >
      {value}
    </span>
  )
}

// Splits `text` on UUIDs, component IDs, and URLs, rendering each as a
// clickable span that copies itself to the clipboard via `onCopy`; these
// are otherwise only useful for cross-referencing other tabs/logs by
// copy-pasting them. `monospace` (default `true`) sets a fixed-width font so
// IDs line up in key-value/table layouts; the log view is already
// monospace end-to-end, so callers there pass `monospace: false` to avoid a
// redundant class.
export function renderCopyableText(
  text: string,
  keyPrefix: string,
  onCopy: (value: string) => void,
  { monospace = true }: { monospace?: boolean } = {},
) {
  const className = monospace ? 'copyable-text copyable-text-mono' : 'copyable-text'
  const nodes: React.ReactNode[] = []
  let lastIndex = 0
  let index = 0
  for (const match of text.matchAll(COPYABLE_TEXT_REGEX)) {
    const matchIndex = match.index ?? 0
    if (matchIndex > lastIndex) {
      nodes.push(text.slice(lastIndex, matchIndex))
    }
    const value = match[0]
    // A URL should read in full and wrap with the surrounding text, not
    // clip with an ellipsis the way a short ID does.
    const isUrl = URL_REGEX.test(value)
    nodes.push(
      <CopyableSpan
        key={`${keyPrefix}-${index++}`}
        value={value}
        className={isUrl ? `${className} copyable-text-wrap` : className}
        onCopy={onCopy}
      />,
    )
    lastIndex = matchIndex + value.length
  }
  if (lastIndex < text.length) {
    nodes.push(text.slice(lastIndex))
  }
  return nodes
}
