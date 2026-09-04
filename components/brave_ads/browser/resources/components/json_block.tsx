/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { renderCopyableText } from '../lib/copyable_text'

// Starts clipped to 2 lines with an ellipsis so a table/log full of these
// doesn't balloon in height by default. "Show more" just removes the clip.
// It doesn't reformat. The minified form is shown throughout, since
// reflowing to pretty-printed multi-line on expand would be a surprising
// shape change for what's meant to be "the same text, just not cut off".
// A click copies the minified form; Cmd/Ctrl+click copies pretty-printed,
// since that's what's actually useful to paste elsewhere. IDs embedded
// within it are still individually click-to-copy via `renderCopyableText`.
export function JsonBlock({ value, keyPrefix, onCopy }: {
  value: unknown
  keyPrefix: string
  onCopy: (value: string) => void
}) {
  const [expanded, setExpanded] = React.useState(false)
  const [isTruncated, setIsTruncated] = React.useState(false)
  const collapsedRef = React.useRef<HTMLPreElement>(null)
  const pretty = JSON.stringify(value, null, 2)
  const compact = JSON.stringify(value)

  React.useLayoutEffect(() => {
    const element = collapsedRef.current
    if (!expanded && element) {
      setIsTruncated(element.scrollHeight > element.clientHeight)
    }
  }, [expanded, compact])

  function onClickToCopy(event: React.MouseEvent) {
    // An embedded ID's own click-to-copy (`renderCopyableText`) already
    // handled this click and copied itself; don't also overwrite the
    // clipboard with the whole block underneath it.
    if ((event.target as HTMLElement).closest('.copyable-text')) {
      return
    }
    onCopy(event.metaKey || event.ctrlKey ? pretty : compact)
  }

  return (
    <span className='json-block'>
      <pre
        ref={expanded ? null : collapsedRef}
        className={`copyable-cell ${expanded ? '' : 'json-block-collapsed'}`}
        title='(Click to copy; Cmd/Ctrl+click to copy pretty-printed)'
        onClick={onClickToCopy}
      >
        {renderCopyableText(compact, keyPrefix, onCopy, { monospace: false })}
      </pre>
      {(expanded || isTruncated) && (
        <span
          className='text-link'
          onClick={() => setExpanded(!expanded)}
        >
          {expanded ? 'Show less' : 'Show more'}
        </span>
      )}
    </span>
  )
}
