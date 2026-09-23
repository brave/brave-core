/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useAppState, useAppActions } from '../lib/app_context'
import { ConditionMatcher } from '../lib/app_store'
import { renderCopyableText } from '../lib/copyable_text'
import { maybeFormatTimestamp, uniqueTableRows } from '../lib/format_time'
import { useCopyToClipboard } from './copy_toast'
import { MatchIcon } from './match_icon'
import { TabHeader } from './tab_header'

// Boilerplate `prefPath` prefixes (see condition_matcher_util.h), greyed out
// so the meaningful part of the path stands out. Both can appear together
// (e.g. `[!]:[virtual]:foo`), hence the loop below rather than a single check.
const GREY_PREF_PATH_PREFIXES = ['[!]:', '[virtual]:']

function renderPrefPath(prefPath: string) {
  const nodes: React.ReactNode[] = []
  let rest = prefPath
  let key = 0
  let matchedPrefix = true
  while (matchedPrefix) {
    matchedPrefix = false
    for (const prefix of GREY_PREF_PATH_PREFIXES) {
      if (rest.startsWith(prefix)) {
        nodes.push(
          <span key={key++} className='diagnostic-muted'>{prefix}</span>,
        )
        rest = rest.slice(prefix.length)
        matchedPrefix = true
        break
      }
    }
  }
  nodes.push(rest)
  return nodes
}

const CONDITION_MATCHER_COLUMNS: Array<keyof ConditionMatcher> = [
  'Pref Path',
  'Condition',
  'Current Value',
  'Matches',
]

// "Current Value" reads better as just "Value" once it's its own column.
// The data key itself stays as-is to match the backend's JSON field name.
const COLUMN_LABELS: Partial<Record<keyof ConditionMatcher, string>> = {
  'Current Value': 'Value',
}

// Long enough to wrap onto multiple lines at an equal-share column width,
// so these are truncated with a tooltip instead.
const TRUNCATED_COLUMNS = new Set<keyof ConditionMatcher>([
  'Pref Path',
  'Condition',
  'Current Value',
])

// Explicit widths so "Matches" (always a short string) doesn't take an equal
// 25% share, leaving more room for the long, truncated columns.
const COLUMN_WIDTH_CLASSES: Partial<Record<keyof ConditionMatcher, string>> = {
  'Pref Path': 'rule-column',
  'Condition': 'rule-column',
  'Current Value': 'value-column',
  'Matches': 'status-column',
}

// Rough character budget per truncated column's width, below which text is
// unlikely to actually be ellipsis-clipped; a tooltip repeating fully
// visible text is just noise.
const APPROX_CHAR_BUDGET: Partial<Record<keyof ConditionMatcher, number>> = {
  'Pref Path': 36,
  'Condition': 36,
  'Current Value': 24,
}

// Matches `kUnknownCurrentValue` in diagnostic_manager.cc.
const UNKNOWN_CURRENT_VALUE = 'Unknown'

// The "[pref path operator]" (does not exist) matcher (see
// condition_matcher_util.h) has no value by design, not by error, so
// "Unknown" there isn't a problem worth calling out the way it is elsewhere.
function isNotOperatorPrefPath(prefPath: string) {
  return prefPath.startsWith('[!]:')
}

// Epoch operators (see epoch_operator_condition_matcher_util.cc) all use a
// "[T<op>]:" prefix and compare elapsed time since a Unix/Windows epoch
// timestamp, unlike every other operator which compares literal values.
function isEpochOperatorCondition(condition: string) {
  return condition.startsWith('[T')
}

function formatValue(header: keyof ConditionMatcher, row: ConditionMatcher) {
  if (header === 'Current Value' &&
      row['Current Value'] === UNKNOWN_CURRENT_VALUE &&
      isNotOperatorPrefPath(row['Pref Path'])) {
    return ''
  }
  return row[header]
}

// Green when it currently matches, muted when it doesn't (often deliberate
// targeting, not a problem), red when the condition itself is
// invalid/malformed, and flagged orange when the pref path couldn't be
// resolved at all; both red and orange indicate a real configuration
// problem rather than an expected non-match.
function matchesClassName(row: ConditionMatcher) {
  if (row['Current Value'] === UNKNOWN_CURRENT_VALUE &&
      !isNotOperatorPrefPath(row['Pref Path'])) {
    return 'condition-matcher-unknown'
  }
  if (row.Matches === 'Invalid') {
    return 'condition-matcher-no-match'
  }
  if (row.Matches === 'Yes') {
    return 'condition-matcher-match'
  }
  return 'diagnostic-muted'
}

// Copyable so a value can be pasted straight into the "Test a condition
// matcher" form's matching input below.
const COPYABLE_COLUMNS = new Set<keyof ConditionMatcher>([
  'Pref Path',
  'Condition',
])

function ConditionMatchersTable({ data }: { data: ConditionMatcher[] }) {
  const copy = useCopyToClipboard()

  return (
    <table>
      <thead>
        <tr>
          {CONDITION_MATCHER_COLUMNS.map((header) => (
            <th
              key={header}
              className={[
                TRUNCATED_COLUMNS.has(header) ? 'truncate-cell' : '',
                COLUMN_WIDTH_CLASSES[header] ?? '',
              ].join(' ').trim()}
            >
              {COLUMN_LABELS[header] ?? header}
            </th>
          ))}
        </tr>
      </thead>
      <tbody>
        {uniqueTableRows(data).map((row, index) => (
          <tr key={index}>
            {CONDITION_MATCHER_COLUMNS.map((header) => {
              const isTruncated = TRUNCATED_COLUMNS.has(header)
              const displayValue = formatValue(header, row)
              const className = [
                isTruncated ? 'truncate-cell' : '',
                COLUMN_WIDTH_CLASSES[header] ?? '',
                header === 'Matches' ? matchesClassName(row) : '',
                header === 'Condition' && row.Matches === 'Invalid'
                  ? 'condition-matcher-no-match'
                  : '',
              ].join(' ').trim()
              const timestampTooltip = maybeFormatTimestamp(String(displayValue))
              const exceedsCharBudget =
                String(displayValue).length > (APPROX_CHAR_BUDGET[header] ?? 0)
              const isCopyable = COPYABLE_COLUMNS.has(header) && displayValue
              const title = timestampTooltip ??
                (isCopyable
                  ? 'Click to copy'
                  : (isTruncated && exceedsCharBudget
                    ? String(displayValue)
                    : undefined))
              return (
                <td
                  key={header}
                  className={isCopyable
                    ? `${className} copyable-cell`.trim()
                    : className}
                  title={title}
                  onClick={isCopyable
                    ? () => copy(String(displayValue))
                    : undefined}
                >
                  {header === 'Pref Path'
                    ? renderPrefPath(String(displayValue))
                    : displayValue}
                </td>
              )
            })}
          </tr>
        ))}
      </tbody>
    </table>
  )
}

function CreativeSection({ creativeInstanceId, matchers }: {
  creativeInstanceId: string
  matchers: ConditionMatcher[]
}) {
  const copy = useCopyToClipboard()
  const matchingCount =
    matchers.filter((matcher) => matcher.Matches === 'Yes').length
  const invalidCount =
    matchers.filter((matcher) => matcher.Matches === 'Invalid').length

  return (
    <div className='content-card'>
      <section className='nested-section'>
        <p className='subsection-title'>
          Creative {renderCopyableText(creativeInstanceId, creativeInstanceId, copy)}{' '}
          <span className='diagnostic-muted'>
            ({matchingCount}/{matchers.length} matching
            {invalidCount > 0 && `, ${invalidCount} invalid`})
          </span>
        </p>
        <ConditionMatchersTable data={matchers} />
      </section>
    </div>
  )
}

// Waits for a pause in typing before testing, rather than on every
// keystroke, so a fast typist doesn't fire a mojo round trip per character.
const VALIDATE_DEBOUNCE_MS = 400

function TestConditionMatcherForm() {
  const actions = useAppActions()
  const persistedForm = useAppState((state) => state.testConditionMatcherForm)

  // The global store notifies listeners a microtask after `update()`, so an
  // input whose `value` is sourced directly from it always lags one
  // microtask behind each keystroke, which resets the cursor to the end on
  // every edit. Local state drives the input synchronously; the global
  // store is only written to for cross-tab persistence, never read back
  // from after this initial mount.
  const [form, setForm] = React.useState(persistedForm)
  const { prefPath, condition, testValue } = form

  function setPrefPath(value: string) {
    const next = { ...form, prefPath: value }
    setForm(next)
    actions.setTestConditionMatcherForm(next)
  }

  function setCondition(value: string) {
    const next = { ...form, condition: value }
    setForm(next)
    actions.setTestConditionMatcherForm(next)
  }

  function setTestValue(value: string) {
    const next = { ...form, testValue: value }
    setForm(next)
    actions.setTestConditionMatcherForm(next)
  }

  const [result, setResult] =
    React.useState<{ currentValue: string, matches: string } | null>(null)

  const isNotOperator = isNotOperatorPrefPath(prefPath)
  // The backend always resolves and returns the real current value for
  // `prefPath` regardless of `condition` (even an empty one), so whether the
  // path exists can be checked independently of whether a condition has been
  // typed yet.
  const currentValue = result?.currentValue
  const pathExists = currentValue !== undefined && currentValue !== UNKNOWN_CURRENT_VALUE
  const showConditionAndTestValue = pathExists && !isNotOperator
  // Epoch operators compare elapsed time since a timestamp rather than the
  // literal value, which isn't obvious from a raw number like "1"; shown
  // next to the field so it's clear what date is actually being tested.
  const testValueTimestamp = isEpochOperatorCondition(condition)
    ? maybeFormatTimestamp(testValue.trim())
    : undefined

  // Cleared rather than merely hidden, so a stale value typed before the
  // path stopped existing (or the "[!]:" operator was added) doesn't
  // silently linger and reappear once the rows do. Only once `result` is
  // populated, not merely because it hasn't resolved yet (e.g. right after
  // this component remounts navigating back to this tab) — otherwise a
  // persisted condition/testValue would be wiped before the debounced check
  // below has a chance to confirm the path still exists.
  React.useEffect(() => {
    if (result !== null && !showConditionAndTestValue) {
      const next = { ...form, condition: '', testValue: '' }
      setForm(next)
      actions.setTestConditionMatcherForm(next)
    }
  }, [showConditionAndTestValue, result])

  React.useEffect(() => {
    // Trimmed only here, right before use, not in the input's `onChange`,
    // which would strip a leading/trailing space the moment it's typed and
    // make it impossible to type a value with one at all.
    const trimmedPrefPath = prefPath.trim()
    if (!trimmedPrefPath) {
      setResult(null)
      return
    }

    let cancelled = false
    const timeoutId = setTimeout(async () => {
      const testResult = await actions.testConditionMatcher(
        trimmedPrefPath, condition.trim(), testValue.trim() || null)
      if (!cancelled) {
        setResult(testResult)
      }
    }, VALIDATE_DEBOUNCE_MS)

    return () => {
      cancelled = true
      clearTimeout(timeoutId)
    }
  }, [prefPath, condition, testValue])

  function onClear() {
    const next = { prefPath: '', condition: '', testValue: '' }
    setForm(next)
    actions.setTestConditionMatcherForm(next)
    setResult(null)
  }

  return (
    <div className='content-card'>
      <h4>
        <span className='title'>Test</span>
        <Button
          size='small'
          kind='plain-faint'
          className={
            prefPath === '' && condition === '' && testValue === ''
              ? 'invisible-reserved'
              : ''
          }
          onClick={onClear}
        >
          Clear
        </Button>
      </h4>
      <p>
        Check whether a pref path and condition match, without needing a real
        creative that declares them. If a Test Value is entered, it overrides
        the device's actual current value for the given pref path.
      </p>
      <section className='key-value-list'>
        <div>
          <span>
            Pref Path
            {pathExists && (
              <span className='diagnostic-muted'>
                {' '}({currentValue})
              </span>
            )}
          </span>
          <span className='test-condition-matcher-value'>
            <input
              className='test-condition-matcher-input'
              value={prefPath}
              autoComplete='off'
              spellCheck={false}
              onChange={(event) => {
                setPrefPath(event.target.value)
                setResult(null)
              }}
            />
            {result && (
              <MatchIcon
                isMatch={isNotOperator ? result.matches === 'Yes' : pathExists}
              />
            )}
          </span>
        </div>
        {showConditionAndTestValue && (
          <div>
            <span>Condition</span>
            <span className='test-condition-matcher-value'>
              <input
                className='test-condition-matcher-input'
                value={condition}
                autoComplete='off'
                spellCheck={false}
                onChange={(event) => setCondition(event.target.value)}
              />
              {condition && result && (
                result.matches === 'Invalid'
                  ? (
                    <Icon
                      name='warning-triangle-filled'
                      className='icon-error'
                      title='Invalid condition'
                    />
                  )
                  : <MatchIcon isMatch={result.matches === 'Yes'} />
              )}
            </span>
          </div>
        )}
        {showConditionAndTestValue && (
          <div>
            <span>Test Value</span>
            <span className='test-condition-matcher-value'>
              <span className='test-condition-matcher-value-column'>
                <input
                  className='test-condition-matcher-input'
                  value={testValue}
                  autoComplete='off'
                  spellCheck={false}
                  placeholder='Leave blank to match against the actual value'
                  onChange={(event) => setTestValue(event.target.value)}
                />
                {testValueTimestamp && (
                  <span className='test-condition-matcher-hint'>
                    {testValueTimestamp}
                  </span>
                )}
              </span>
            </span>
          </div>
        )}
      </section>
    </div>
  )
}

function groupByCreativeInstanceId(conditionMatchers: ConditionMatcher[]) {
  const groups = new Map<string, ConditionMatcher[]>()
  for (const matcher of conditionMatchers) {
    const creativeInstanceId = matcher['Creative Instance ID']
    const group = groups.get(creativeInstanceId)
    if (group) {
      group.push(matcher)
    } else {
      groups.set(creativeInstanceId, [matcher])
    }
  }
  return groups
}

export function ConditionMatchers() {
  const actions = useAppActions()
  const conditionMatchers = useAppState((state) => state.conditionMatchers)
  const adsInternalsVerboseModeEnabled =
    useAppState((state) => state.adsInternalsVerboseModeEnabled)
  const diagnosticEntries = useAppState((state) => state.diagnosticEntries)
  const newTabPageAdsEnabled = diagnosticEntries.find(
    (entry) => entry.name === 'New tab page ads shown',
  )?.value === 'true'

  React.useEffect(() => {
    actions.loadAdsInternals()
    actions.loadDiagnostics()
  }, [])

  const groups = React.useMemo(
    () => groupByCreativeInstanceId(conditionMatchers),
    [conditionMatchers],
  )

  return (
    <>
      <TabHeader
        title='Condition matchers'
        description="Rules that decide whether an ad is eligible to be
          shown, and whether each rule currently matches on this device."
        onRefresh={actions.loadAdsInternals}
      />

      {adsInternalsVerboseModeEnabled && <TestConditionMatcherForm />}

      <div className='card-group'>
        <div className='content-card'>
          <h4>
            <span className='title'>
              New Tab Page ads{' '}
              <span className='diagnostic-muted'>({groups.size})</span>
            </span>
          </h4>
          {newTabPageAdsEnabled && conditionMatchers.length === 0 && (
            <p>No creatives found.</p>
          )}
        </div>

        {[...groups.entries()].map(([creativeInstanceId, matchers]) => (
          <CreativeSection
            key={creativeInstanceId}
            creativeInstanceId={creativeInstanceId}
            matchers={matchers}
          />
        ))}
      </div>
    </>
  )
}
