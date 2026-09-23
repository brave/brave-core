/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { useAppState, useAppActions } from '../lib/app_context'
import { renderCopyableText } from '../lib/copyable_text'
import { useCopyToClipboard, useToast } from './copy_toast'
import { JsonBlock } from './json_block'
import { TabHeader } from './tab_header'

import { style } from './logs.style'

// Rendering every fetched line as its own DOM node made the accessible name
// for the log container grow unbounded on a large log, which was slow to
// compute on every re-render. A virtualized (react-window) rendering was
// tried instead, but its row-height estimation for wrapped multi-line
// messages produced overlapping, garbled text that was worse than this cap;
// plain rendering with a cap this large avoids both problems in practice.
// Use Download for the complete log.
const MAX_RENDERED_LINES = 1000

// Matches the "Response Code: 201 Created" line from
// `url_response_string_util.cc`'s "URL Response:" log entry. Also matches a
// negative code (e.g. "Response Code: -1"), which is a net:: error rather
// than a real HTTP status, sent when the request never got a response at
// all.
const RESPONSE_CODE_REGEX = /Response Code: (-?\d+)/

// 2xx/3xx are successful or redirected responses, 4xx are client errors; any
// other code, including 5xx and a negative net:: error, is a failure, and
// colored the same way so it isn't left uncolored just for not matching a
// recognized 2xx-4xx range.
function responseCodeClassName(line: string) {
  const match = line.match(RESPONSE_CODE_REGEX)
  if (!match) {
    return null
  }
  switch (match[1][0]) {
    case '2':
    case '3':
      return 'log-line-response-success'
    case '4':
      return 'log-line-response-client-error'
    case '5':
      return 'log-line-response-server-error'
    default:
      return 'log-line-response-error'
  }
}

// Not every real failure is logged at ERROR level (e.g. some Rewards
// diagnostic log lines report "Failed to ..." at a lower severity), so
// matching the message text catches those too, not just the level tag.
function isErrorLine(line: string) {
  return line.includes('ERROR') || line.includes('Failed')
}

// Highlights ERROR/WARNING lines and bolds request/response markers
// on-screen only; the downloaded log (see `download()` below) is always
// fetched and written out as plain text, unaffected by this.
function logLineClassName(line: string) {
  if (isErrorLine(line)) {
    return 'log-line-error'
  }
  if (line.includes('WARNING')) {
    return 'log-line-warning'
  }
  const responseCodeClass = responseCodeClassName(line)
  if (responseCodeClass) {
    return responseCodeClass
  }
  if (line.includes('[ REQUEST ]') || line.includes('[ RESPONSE ]')) {
    return 'log-line-request-response'
  }
  return ''
}

// The log source separates entries with a plain-text dash rule; rendered as
// a real divider on-screen only, same on-screen-only rationale as above.
function isDividerLine(line: string) {
  return /^-{10,}$/.test(line.trim())
}

// Each line starts "[<timestamp>:<LEVEL>:<file>(<line>)] " where <timestamp>
// is formatted by `DiagnosticLog::FormatTime()` (see
// components/brave_rewards/content/diagnostic_log.cc) as e.g.
// "Aug 20, 2026 9:35:03.123 AM GMT"; always GMT regardless of local time
// zone, so "today" below is computed in GMT too.
const LOG_LINE_DATE_REGEX = /^\[([A-Za-z]{3} \d{1,2}, \d{4})/

function isLogLineFromToday(line: string) {
  const match = line.match(LOG_LINE_DATE_REGEX)
  if (!match) {
    // Unrecognised format (e.g. a wrapped continuation line); don't grey
    // out something we can't actually date.
    return true
  }

  const lineDate = new Date(`${match[1]} UTC`)
  const today = new Date()
  return lineDate.getUTCFullYear() === today.getUTCFullYear() &&
    lineDate.getUTCMonth() === today.getUTCMonth() &&
    lineDate.getUTCDate() === today.getUTCDate()
}

// Splits a line into its "[<timestamp>] " prefix and message. The
// LEVEL:file(line) part is dropped from the colored view (color-coding
// already conveys ERROR/WARNING, and the file/line is rarely useful at a
// glance); switch to Plain text to see it, e.g. to Cmd+F for it by name.
// "AM"/"PM" is only present at all for a 12-hour clock; a 24-hour clock
// (see `DiagnosticLog::FormatTime()`) omits it entirely rather than
// forcing one on someone who doesn't use it.
const LOG_LINE_REGEX =
  /^\[([A-Za-z]{3} \d{1,2}, \d{4} \d{1,2}:\d{2}:\d{2}\.\d+(?: [AP]M)? GMT):(?:ERROR|INFO|VERBOSE\d+):[^\]]+\] (.*)$/s

function splitLogLine(line: string) {
  const match = line.match(LOG_LINE_REGEX)
  if (!match) {
    return null
  }
  const [, timestamp, message] = match
  return { prefix: `[${timestamp}] `, message }
}

// Finds a JSON value trailing a message like 'Response: {"id":"...", ...}',
// so it can be pretty-printed instead of shown as one unreadable line.
function extractTrailingJson(message: string) {
  const match = message.match(/[[{]/)
  if (!match || match.index === undefined) {
    return null
  }
  const prefix = message.slice(0, match.index)
  try {
    return { prefix, value: JSON.parse(message.slice(match.index)) }
  } catch {
    return null
  }
}

export function Logs() {
  const actions = useAppActions()
  const log = useAppState((state) => state.log)
  const verboseLoggingEnabled = useAppState(
    (state) => state.verboseLoggingEnabled,
  )
  const errorsOnly = useAppState((state) => state.errorsOnlyEnabled)
  // Auto scroll only ever has new content to follow once the log is
  // actually being refreshed.
  const autoRefreshEnabled = useAppState((state) => state.autoRefreshEnabled)

  const copy = useCopyToClipboard()
  const showToast = useToast()

  const textAreaRef = React.useRef<HTMLDivElement>(null)
  // Sticks new log lines to the bottom of the view while on, until the user
  // manually scrolls away from the bottom, which switches it off; unlike the
  // other toggles below, scrolling back to the bottom by hand doesn't turn
  // it back on by itself, the switch does.
  const [autoScroll, setAutoScroll] = React.useState(true)
  // Auto scroll is inert without auto-refresh (nothing new to follow), so
  // treat the two together as the single "should the view track new
  // content" condition used throughout below.
  const followingBottom = autoScroll && autoRefreshEnabled
  const [showVerboseDialog, setShowVerboseDialog] = React.useState(false)
  // Local, on-screen-only preference (not persisted); shows each line
  // exactly as logged, with no color-coding or LEVEL:file(line) stripping.
  const [rawMode, setRawMode] = React.useState(false)
  // Set by "View in full log" below; consumed once the full log's lines are
  // rendered, to scroll straight to the line that was clicked from the
  // errors-only view rather than leaving the view at the bottom. A ref
  // wouldn't work here since `visibleLines` below needs to react to it (to
  // make sure the target line is actually within the rendered window).
  const [jumpToLineIndex, setJumpToLineIndex] = React.useState<number | null>(
    null,
  )
  // Marks the line last jumped to via "View in full log" with a dot, so it's
  // easy to spot after the view re-centers on it; cleared the next time the
  // user actually scrolls (not the jump's own scroll, guarded by
  // `jumpedAtRef` below), since that's when the mark has served its purpose.
  const [markedLineIndex, setMarkedLineIndex] = React.useState<number | null>(
    null,
  )
  const jumpedAtRef = React.useRef(0)
  // Last known scroll position while not following the bottom; some
  // browsers keep a scroll container that's already at its max scrollTop
  // pinned to the bottom as content is appended below, even with nothing
  // here asking for that. Re-pinning to this after every content change
  // (see the layout effect below) overrides that regardless of the exact
  // trigger.
  const scrollTopRef = React.useRef(0)
  // Whether the previous render was following the bottom, so the layout
  // effect below can tell "just stopped following" (sync the pin from the
  // live position) apart from "still not following, content changed"
  // (restore the pin), rather than re-pinning to a stale position from
  // before auto-scroll was last turned back on.
  const wasFollowingBottomRef = React.useRef(followingBottom)
  // A fixed calc(dvh - Npx) in CSS only accounts for whatever happens to
  // render above `.log-view` right now, and silently breaks again the next
  // time that changes (header/description wrapping, other tabs' controls,
  // etc.); measuring its own actual position keeps it correct regardless.
  const [logViewHeight, setLogViewHeight] = React.useState<number>()

  React.useEffect(() => {
    actions.loadLog()
  }, [])

  // Layout effect (fires before paint), not a plain effect, so the first
  // paint already has the correct height instead of briefly flashing full
  // natural content height. Re-measures on errorsOnly/verboseLoggingEnabled/
  // rawMode too, since any of them can add or remove the "Showing the most
  // recent N of M lines" banner above this element (see `truncatedCount`
  // below), which shifts this element's own top position.
  React.useLayoutEffect(() => {
    const elem = textAreaRef.current
    if (!elem) {
      return
    }
    function updateHeight() {
      const top = elem!.getBoundingClientRect().top
      // Matches `main`'s own bottom padding, so this ends with the same
      // breathing room every other tab's content has.
      const bottomPadding = 32
      setLogViewHeight(Math.max(0, window.innerHeight - top - bottomPadding))
    }
    updateHeight()
    window.addEventListener('resize', updateHeight)
    return () => window.removeEventListener('resize', updateHeight)
  }, [errorsOnly, verboseLoggingEnabled, rawMode])

  // Overrides a browser's own attempt to keep an already-bottomed-out
  // scroll container stuck to the bottom as new content is appended, since
  // that only checks scrollTop against scrollHeight, not this component's
  // own auto-scroll toggle.
  React.useLayoutEffect(() => {
    const elem = textAreaRef.current
    if (!elem) {
      return
    }
    if (followingBottom) {
      wasFollowingBottomRef.current = true
      return
    }
    if (wasFollowingBottomRef.current) {
      // Just stopped following; the current (likely bottom) position is
      // exactly where to stay pinned, not whatever this last held from
      // before auto-scroll was previously turned back on.
      scrollTopRef.current = elem.scrollTop
      wasFollowingBottomRef.current = false
    } else {
      elem.scrollTop = scrollTopRef.current
    }
  }, [followingBottom, log])

  // A specific line was requested via "View in full log"; jump to it once
  // it's actually rendered, rather than leaving the view wherever
  // auto-scroll (below) happened to land it.
  React.useEffect(() => {
    const elem = textAreaRef.current
    if (!elem || jumpToLineIndex === null) {
      return
    }
    if (errorsOnly) {
      // `setErrorsOnlyEnabled(false)` triggered this same effect run before
      // its own state update had settled; wait for the render where
      // errors-only has actually turned off rather than searching the
      // still-filtered view (which would find a false match and clear
      // `jumpToLineIndex` before the real jump ever happens).
      return
    }
    const target = elem.querySelector(`[data-line-index="${jumpToLineIndex}"]`)
    setJumpToLineIndex(null)
    if (target) {
      // The user asked to jump to a specific line, not to keep tailing new
      // lines from here.
      setAutoScroll(false)
      target.scrollIntoView({ block: 'center' })
      setMarkedLineIndex(jumpToLineIndex)
      jumpedAtRef.current = performance.now()
    }
  }, [errorsOnly, jumpToLineIndex])

  // Toggling errors-only/verbose/plain-text, or new lines arriving, can add
  // hundreds of lines to `.log-view` at once; a MutationObserver (unlike a
  // one-shot scrollTo right after the state change) only fires once those
  // DOM nodes are actually applied, so `scrollHeight` is guaranteed correct
  // when read, rather than guessing how many frames layout needs to settle.
  React.useEffect(() => {
    const elem = textAreaRef.current
    if (!elem || !followingBottom) {
      return
    }
    function scrollToBottom() {
      // Guards the scroll event this generates (see `onLogViewScroll`).
      jumpedAtRef.current = performance.now()
      elem!.scrollTo({ top: elem!.scrollHeight })
      // A large mutation batch can itself trigger a second, smaller layout
      // pass (e.g. text wrapping settling) after this callback already
      // read `scrollHeight`; catches that residual growth. Confirmed
      // necessary: removing this regressed the bug it fixes.
      requestAnimationFrame(() => {
        jumpedAtRef.current = performance.now()
        elem!.scrollTo({ top: elem!.scrollHeight })
      })
    }
    // Also reruns on `logViewHeight` (see the layout effect above): a
    // window resize changes `.log-view`'s own height, not its content, so
    // the MutationObserver alone never fires for it; without this,
    // resizing leaves the view exactly where it was relative to the box's
    // old (taller or shorter) size instead of at the new bottom. Depending
    // on the already-applied height (rather than the resize event itself)
    // avoids reading `scrollHeight` before React has actually applied it.
    scrollToBottom()
    const observer = new MutationObserver(scrollToBottom)
    observer.observe(
      elem, { childList: true, subtree: true, characterData: true },
    )
    return () => observer.disconnect()
  }, [followingBottom, logViewHeight])

  function onLogViewScroll() {
    const elem = textAreaRef.current
    if (!elem) {
      return
    }
    // Ignore the scroll event our own programmatic scroll (auto-scroll's
    // `scrollTo`, or a jump's `scrollIntoView`) generates; only a scroll
    // after that has settled means the user actually scrolled.
    if (performance.now() - jumpedAtRef.current <= 300) {
      return
    }
    // Tracked regardless of auto-scroll state, so the re-pinning layout
    // effect above always has an up-to-date position to restore.
    scrollTopRef.current = elem.scrollTop
    const distanceFromBottom =
      elem.scrollHeight - elem.scrollTop - elem.clientHeight
    // Auto scroll is inert (and hidden) without auto-refresh; don't let
    // scrolling around while it's off silently flip its value for when
    // auto-refresh comes back on.
    if (autoRefreshEnabled && distanceFromBottom >= 4) {
      setAutoScroll(false)
    }
    if (markedLineIndex !== null) {
      setMarkedLineIndex(null)
    }
  }

  // `String.split('\n')` always yields a trailing empty element for a
  // trailing newline (or a single empty element for an empty string
  // entirely, e.g. right after clearing). Drop it so it doesn't render as
  // a numbered line with no content.
  const allLinesUnfiltered = React.useMemo(() => {
    const lines = log.split('\n')
    if (lines.length > 0 && lines[lines.length - 1] === '') {
      lines.pop()
    }
    return lines
  }, [log])
  // "View in full log" only actually finds its target within the full,
  // unfiltered log's own last `MAX_RENDERED_LINES` (see `visibleLines`
  // below); for an error line further back than that, don't show a jump
  // icon that wouldn't do anything.
  const fullLogRenderWindowStart =
    Math.max(0, allLinesUnfiltered.length - MAX_RENDERED_LINES)
  const allLines = React.useMemo(
    () => allLinesUnfiltered
      .map((line, originalIndex) => ({ line, originalIndex }))
      .filter(({ line }) => !errorsOnly || isErrorLine(line)),
    [allLinesUnfiltered, errorsOnly],
  )
  // Where the render window starts within `allLines`. Tracks the tail while
  // following the bottom, but frozen while paused, so a batch of new lines
  // arriving only appends below rather than evicting already-rendered lines
  // out from under someone who scrolled up to read them. Clamped down if
  // `allLines` itself shrinks (e.g. toggling Errors only) so it never points
  // past the end.
  const [windowStart, setWindowStart] = React.useState(
    () => Math.max(0, allLines.length - MAX_RENDERED_LINES),
  )
  React.useEffect(() => {
    const maxStart = Math.max(0, allLines.length - MAX_RENDERED_LINES)
    if (followingBottom) {
      setWindowStart(maxStart)
    } else {
      setWindowStart((current) => Math.min(current, maxStart))
    }
  }, [followingBottom, allLines.length])
  // Normally just the tail, but a pending jump target might be further back
  // than that; make sure the window actually contains it rather than
  // silently landing on the bottom instead (see `jumpToLineIndex`).
  const jumpTargetPosition = jumpToLineIndex === null
    ? -1
    : allLines.findIndex(({ originalIndex }) => originalIndex === jumpToLineIndex)
  const truncatedCount = windowStart
  const visibleLines = truncatedCount <= 0
    ? allLines
    : jumpTargetPosition !== -1 && jumpTargetPosition < windowStart
      ? allLines.slice(
          jumpTargetPosition,
          jumpTargetPosition + MAX_RENDERED_LINES,
        )
      : allLines.slice(windowStart)

  // Classifying a line (regex matching, a JSON.parse attempt) is wasted work
  // if it reruns on every render regardless of whether the line itself
  // changed; memoized here so it only reruns when the visible window or
  // rawMode actually changes.
  const renderableLines = React.useMemo(() => visibleLines.map(
    ({ line, originalIndex }) => {
      if (!rawMode && isDividerLine(line)) {
        return {
          originalIndex, line, isDivider: true, split: null,
          messageClassName: '', pastDay: false, messageText: line,
          trailingJson: null,
        }
      }
      const split = rawMode ? null : splitLogLine(line)
      const messageClassName = rawMode ? '' : logLineClassName(line)
      const pastDay = !rawMode && messageClassName === '' &&
        !isLogLineFromToday(line)
      const messageText = split ? split.message : line
      const trailingJson = rawMode ? null : extractTrailingJson(messageText)
      return {
        originalIndex, line, isDivider: false, split,
        messageClassName, pastDay, messageText, trailingJson,
      }
    }), [visibleLines, rawMode])

  function download() {
    actions.fetchFullLog().then((fullLog) => {
      const content =
        'WARNING: This log file may contain sensitive data. Be careful who ' +
        'you share it with.\n\n' + fullLog
      const filename = 'brave_ads_internals_log.txt'
      const element = document.createElement('a')
      element.setAttribute(
        'href',
        'data:text/plain;charset=utf-8,' + encodeURIComponent(content),
      )
      element.setAttribute('download', filename)
      element.style.display = 'none'
      document.body.appendChild(element)
      element.click()
      document.body.removeChild(element)
    })
  }

  return (
    <div
      className='card-group'
      data-css-scope={style.scope}
    >
      <TabHeader
        title='Logs'
        description='The diagnostic log for ads and related activity.'
        onRefresh={actions.loadLog}
      />

      <div className='content-card'>
        <h4>
          <Toggle
            size='small'
            checked={verboseLoggingEnabled}
            onChange={() => setShowVerboseDialog(true)}
          >
            Verbose mode
          </Toggle>
          <Toggle
            size='small'
            checked={errorsOnly}
            onChange={() => actions.setErrorsOnlyEnabled(!errorsOnly)}
          >
            Errors only
          </Toggle>
          <Toggle
            size='small'
            checked={rawMode}
            onChange={() => setRawMode(!rawMode)}
          >
            Plain text
          </Toggle>
          {autoRefreshEnabled && (
            <Toggle
              size='small'
              checked={autoScroll}
              onChange={() => setAutoScroll(!autoScroll)}
            >
              Auto scroll
            </Toggle>
          )}
          <span className='title' />
          {log !== '' && (
            <Button
              size='small'
              onClick={download}
            >
              Download
            </Button>
          )}
          <Button
            size='small'
            onClick={async () => {
              await actions.clearLog()
              showToast('Log cleared.')
            }}
          >
            Clear
          </Button>
        </h4>

        <p>
          {truncatedCount > 0
            ? <>
                Showing the most recent {MAX_RENDERED_LINES} lines.
                Download the log to see the rest.
              </>
            : `Showing ${allLines.length} line${
                allLines.length === 1 ? '' : 's'
              }.`}
        </p>

        <div
          ref={textAreaRef}
          className='log-view'
          style={{ height: logViewHeight }}
          onScroll={onLogViewScroll}
        >
          {visibleLines.length === 0 && (
            <div>
              {errorsOnly ? 'No errors have been logged.' : 'The log is empty.'}
            </div>
          )}
          {renderableLines.map(({
            line, originalIndex, isDivider, split, messageClassName,
            pastDay, messageText, trailingJson,
          }, index) => {
            if (isDivider) {
              return <hr key={originalIndex} className='log-divider' />
            }
            return (
              <div key={originalIndex} data-line-index={originalIndex}>
                {originalIndex === markedLineIndex && (
                  <Icon name='dot' className='log-line-marker' />
                )}
                <span className='log-line-number'>
                  {originalIndex + 1}
                </span>
                {split && (
                  <span className='log-line-prefix'>{split.prefix}</span>
                )}
                <span
                  className={pastDay
                    ? 'diagnostic-muted'
                    : messageClassName}
                >
                  {trailingJson
                    ? <>
                        {renderCopyableText(
                          trailingJson.prefix,
                          `${index}-prefix`,
                          copy,
                          { monospace: false },
                        )}
                        <JsonBlock
                          value={trailingJson.value}
                          keyPrefix={`${index}-json`}
                          onCopy={copy}
                        />
                      </>
                    : renderCopyableText(messageText, String(index), copy, {
                        monospace: false,
                      })}
                </span>
                {!rawMode && errorsOnly &&
                  originalIndex >= fullLogRenderWindowStart &&
                  isErrorLine(line) && (
                  <span
                    className='log-line-jump-to-full'
                    title='View in full log'
                    onClick={() => {
                      setJumpToLineIndex(originalIndex)
                      actions.setErrorsOnlyEnabled(false)
                    }}
                  >
                    ↗️
                  </span>
                )}
              </div>
            )
          })}
        </div>
      </div>

      <Dialog
        isOpen={showVerboseDialog}
        onClose={() => setShowVerboseDialog(false)}
        backdropClickCloses={false}
      >
        <div slot='title'>Brave Rewards Verbose Logging</div>
        <div className='verbose-logging-info'>
          <Icon name='warning-triangle-filled' />
          <div>
            Enables detailed logging of Brave Rewards system events to a log
            file stored on your device. Please note that this log file could
            include information such as browsing history and credentials such as
            passwords and access tokens depending on your activity. Please do
            not share it unless asked to by Brave staff.
          </div>
        </div>
        <div slot='actions'>
          <Button
            kind='plain-faint'
            onClick={() => setShowVerboseDialog(false)}
          >
            Cancel
          </Button>
          <Button
            onClick={() => {
              setShowVerboseDialog(false)
              actions.toggleVerboseLoggingAndRestart()
            }}
          >
            {verboseLoggingEnabled
              ? 'Disable and Restart'
              : 'Enable and Restart'}
          </Button>
        </div>
      </Dialog>
    </div>
  )
}
