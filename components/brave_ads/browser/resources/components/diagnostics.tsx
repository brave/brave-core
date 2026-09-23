/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import Link from '@brave/leo/react/link'

import { useAppState, useAppActions } from '../lib/app_context'
import { DiagnosticEntry } from '../lib/app_store'
import { isValidUuidV4 } from '../lib/copyable_text'
import {
  getDiagnosticValue,
  resolveDiagnosticName,
  resolveDiagnosticValue,
} from '../lib/diagnostics'
import { MatchIcon } from './match_icon'
import { useCopyToClipboard } from './copy_toast'
import { TabHeader } from './tab_header'

// The Device ID is worth hiding from screen-shares/screenshots by default,
// since it's a stable per-device identifier. Rendered as CSS dots rather
// than a bullet-character string, so it looks the same across fonts;
// clicking it reveals the real value in place.
const MASKED_DIAGNOSTIC_DOTS = Array.from({ length: 8 })

// Rewards-only state; hidden entirely (not just muted) when Rewards isn't
// joined.
const REWARDS_ONLY_ENTRY_NAMES = new Set([
  // Geo-targeting for ad serving, which doesn't apply to non-Rewards users.
  'Subdivision code',
])

// Shown on the Ad Formats tab instead, so excluded here entirely (including
// from "Other") rather than duplicated.
const AD_FORMATS_TAB_ENTRY_NAMES = new Set([
  'Sponsored ads enabled',
  'New tab page ads shown',
  'Notification ads enabled',
])

// Shown on the User Attention tab instead, so excluded here entirely
// (including from "Other") rather than duplicated.
const USER_ATTENTION_ENTRY_NAMES = new Set(['Last unidle time'])

// Shown on the Resources tab instead, so excluded here entirely (including
// from "Other") rather than duplicated. Only these two: the rest of that
// tab's entries (resource load state, catalog schema version/next update)
// come from the dedicated `resourcesEntries` diagnostics key, so they never
// appear in this shared list to begin with.
const RESOURCE_ENTRY_NAMES = new Set([
  'Catalog ID',
  'Catalog last updated',
])

// Diagnostic entries whose raw "true"/"false" value should be rendered as a
// human-readable label. `isProblem` marks the value that's worth calling out
// as a problem on brave://ads-internals; `isSuccess` marks the value worth
// calling out as a healthy state, so both stand out from ordinary values.
const DIAGNOSTIC_VALUE_LABELS: Record<
  string,
  Record<string, { label: string; isProblem: boolean; isSuccess: boolean }>
> = {
  'Ads initialized': {
    'true': { label: 'Running', isProblem: false, isSuccess: true },
    'false': { label: 'FAILED', isProblem: true, isSuccess: false },
  },
}

function displayDiagnosticValue(name: string, value: string) {
  return resolveDiagnosticValue(name, value, DIAGNOSTIC_VALUE_LABELS)
}

// "N/A"/"Never"/"None" mean there's nothing to see yet, or a healthy empty
// state; grey these out so they don't compete with actual problem states for
// attention.
function isDiagnosticValueUnavailable(rawValue: string) {
  return rawValue === 'N/A' || rawValue === 'Never' || rawValue === 'None'
}

// Once an entry is grouped under a section (e.g. "Catalog"), repeating the
// section name in every row reads as noise: "Catalog ID" inside "Catalog"
// says nothing "ID" doesn't. Shortens the row label only; the lookup key
// used for sectioning/value-formatting above is still the full backend name.
const DIAGNOSTIC_NAME_LABELS: Record<string, string> = {
  // Names the actual standard this code follows.
  'Subdivision code': 'ISO 3166-2 subdivision code',
  'Ads initialized': 'Status',
}

function displayDiagnosticName(name: string) {
  return resolveDiagnosticName(name, DIAGNOSTIC_NAME_LABELS)
}

// Device "Language"/"Country" read oddly as two separate rows next to
// "Variations (Griffin) country" (a different, server-determined country);
// combine them into one "en-GB" style "Device locale" row so it reads as a
// single fact and isn't mistaken for the variations country.
function combineLanguageAndCountry(
  entries: DiagnosticEntry[],
): DiagnosticEntry[] {
  const language = entries.find((entry) => entry.name === 'Language')
  const country = entries.find((entry) => entry.name === 'Country')
  if (!language || !country) {
    return entries
  }

  return entries
    .filter((entry) => entry.name !== 'Country')
    .map((entry) => entry.name === 'Language'
      ? { name: 'Device locale', value: `${language.value}-${country.value}` }
      : entry)
}

// Groups the flat entry list returned by `DiagnosticManager` into sections so
// related facts aren't lost in one long list. Entries not named here still
// render, under "Other", so a newly added diagnostic entry is never silently
// dropped.
const DIAGNOSTIC_SECTIONS: Array<{
  title: string
  names: string[]
}> = [
  {
    title: 'Service',
    names: ['Ads initialized'],
  },
  {
    title: 'Language & Region',
    names: [
      'Device locale',
      'Variations (Griffin) country code',
      'Subdivision code',
    ],
  },
]

function DiagnosticSection(
  { title, entries }: {
    title: string
    entries: DiagnosticEntry[]
  },
) {
  if (entries.length === 0) {
    return null
  }

  return (
    <div className='content-card'>
      <h4>
        <span className='title'>{title}</span>
      </h4>
      <section className='key-value-list'>
        {entries.map((entry) => {
          const { label, isProblem, isSuccess } =
            displayDiagnosticValue(entry.name, entry.value)
          const className = isProblem
            ? 'diagnostic-problem'
            : isDiagnosticValueUnavailable(entry.value)
              ? 'diagnostic-muted'
              : isSuccess
                ? 'diagnostic-success'
                : ''
          return (
            <div key={entry.name}>
              <span>{displayDiagnosticName(entry.name)}</span>
              <span className={className}>{label}</span>
            </div>
          )
        })}
      </section>
    </div>
  )
}

// Renders "Device ID" as its own masked row inside the top card (see
// `TabHeader`'s children in `Diagnostics` below), separate from the
// generic `DiagnosticSection` list since it needs its own reveal-on-click
// state rather than living inside a titled section.
function DeviceIdRow() {
  const rawEntries = useAppState((state) => state.diagnosticEntries)
  const rewardsEnabled = useAppState((state) => state.rewardsEnabled)
  const [revealed, setRevealed] = React.useState(false)

  const deviceId = getDiagnosticValue(rawEntries, 'Device ID')

  // Only used to compute the rotating anonymity-set hash sent with ad
  // catalog/confirmation requests, which don't happen for non-Rewards users.
  if (!rewardsEnabled || deviceId === undefined) {
    return null
  }

  return (
    <section className='key-value-list'>
      <div>
        <span>Device ID</span>
        {revealed
          ? <span>{deviceId}</span>
          : (
            <button
              type='button'
              className='diagnostic-masked'
              title='Click to reveal'
              onClick={() => setRevealed(true)}
            >
              {MASKED_DIAGNOSTIC_DOTS.map((_, dotIndex) => (
                <span key={dotIndex} className='diagnostic-masked-dot' />
              ))}
            </button>
          )}
      </div>
    </section>
  )
}

// A v4 UUID is always exactly this many characters; sizing the input to fit
// exactly means neither wasted space nor scrolling to see the full value.
const DIAGNOSTIC_ID_MAX_LENGTH = 36

const DIAGNOSTIC_ID_DIALOG_COPY = {
  generate: {
    title: 'Generate a new diagnostic ID?',
    body: 'The current one will be replaced with a new, random one.',
    confirmLabel: 'Generate',
  },
  clear: {
    title: 'Clear the diagnostic ID?',
    body: 'The diagnostic ID will be cleared. This won’t affect any ' +
      'diagnostics data you’ve already reported.',
    confirmLabel: 'Clear',
  },
}

function DiagnosticIdRow() {
  const actions = useAppActions()
  const diagnosticId = useAppState((state) => state.diagnosticId)
  const isValid = isValidUuidV4(diagnosticId)
  const copy = useCopyToClipboard()
  const rewardsEnabled = useAppState((state) => state.rewardsEnabled)
  const [pendingAction, setPendingAction] =
    React.useState<'generate' | 'clear' | null>(null)

  if (!rewardsEnabled) {
    return null
  }

  function generate() {
    actions.setDiagnosticId(crypto.randomUUID())
  }

  function clear() {
    actions.setDiagnosticId('')
  }

  return (
    <div className='content-card'>
      <h4>
        <span className='title'>Diagnostics</span>
        {diagnosticId !== '' && (
          <span className='header-end-action'>
            <Button
              size='small'
              kind='plain-faint'
              onClick={() => setPendingAction('clear')}
            >
              Clear
            </Button>
          </span>
        )}
      </h4>
      <h4>
        <span className='id-input-group'>
          <span>ID</span>
          <input
            className='diagnostic-id-input'
            value={diagnosticId}
            maxLength={DIAGNOSTIC_ID_MAX_LENGTH}
            autoComplete='off'
            spellCheck={false}
            onChange={(event) => {
              actions.setDiagnosticId(event.target.value)
            }}
            onBlur={(event) => {
              actions.setDiagnosticId(event.target.value.trim())
            }}
          />
          {isValid && (
            <span className='fixed-flex-item'>
              <Button
                size='small'
                kind='plain-faint'
                title='Click to copy'
                aria-label='Copy diagnostic ID'
                onClick={() => copy(diagnosticId)}
              >
                <Icon name='copy' />
              </Button>
            </span>
          )}
          {diagnosticId !== '' && <MatchIcon isMatch={isValid} />}
        </span>
        <Button
          size='small'
          kind='plain-faint'
          onClick={() => {
            if (diagnosticId) {
              setPendingAction('generate')
            } else {
              generate()
            }
          }}
        >
          Generate ID
        </Button>
      </h4>
      <Dialog
        isOpen={pendingAction !== null}
        onClose={() => setPendingAction(null)}
      >
        {pendingAction && (
          <>
            <div slot='title'>
              {DIAGNOSTIC_ID_DIALOG_COPY[pendingAction].title}
            </div>
            <div>{DIAGNOSTIC_ID_DIALOG_COPY[pendingAction].body}</div>
            <div slot='actions'>
              <Button
                kind='plain-faint'
                onClick={() => setPendingAction(null)}
              >
                Cancel
              </Button>
              <Button
                onClick={() => {
                  const action = pendingAction
                  setPendingAction(null)
                  if (action === 'generate') {
                    generate()
                  } else {
                    clear()
                  }
                }}
              >
                {DIAGNOSTIC_ID_DIALOG_COPY[pendingAction].confirmLabel}
              </Button>
            </div>
          </>
        )}
      </Dialog>
    </div>
  )
}

export function Diagnostics() {
  const actions = useAppActions()
  const rawEntries = useAppState((state) => state.diagnosticEntries)
  const isInitialized = useAppState((state) => state.isInitialized)
  const rewardsEnabled = useAppState((state) => state.rewardsEnabled)
  const variationsCountryCode = useAppState(
    (state) => state.variationsCountryCode,
  )
  React.useEffect(() => {
    actions.loadDiagnostics()
  }, [])

  // `isInitialized` defaults to `false` until the first `loadDiagnostics`
  // response arrives, which would otherwise flash "Status: FAILED" on every
  // page load rather than only when startup genuinely fails; only show it
  // once real diagnostic entries have actually loaded.
  const hasLoadedDiagnostics = rawEntries.length > 0
  const entries = combineLanguageAndCountry([
    ...rawEntries,
    ...(hasLoadedDiagnostics
      ? [{ name: 'Ads initialized', value: String(isInitialized) }]
      : []),
    {
      name: 'Variations (Griffin) country code',
      value: variationsCountryCode || 'N/A',
    },
  ])

  // Every named entry is excluded from "Other", even when its section is hidden
  // because Rewards is disabled; it should disappear, not resurface.
  const named = new Set([
    'Device ID',
    ...DIAGNOSTIC_SECTIONS.flatMap((section) => section.names),
    ...RESOURCE_ENTRY_NAMES,
    ...USER_ATTENTION_ENTRY_NAMES,
    ...AD_FORMATS_TAB_ENTRY_NAMES,
  ])

  const sections = DIAGNOSTIC_SECTIONS.map((section) => ({
    title: section.title,
    // Look up entries in `section.names` order rather than filtering the flat
    // list; filtering would keep the backend's enum-derived order, silently
    // ignoring the order declared for this section.
    entries: section.names
      .map((name) => entries.find((entry) => entry.name === name))
      .filter((entry): entry is DiagnosticEntry => entry !== undefined &&
        (rewardsEnabled || !REWARDS_ONLY_ENTRY_NAMES.has(entry.name))),
  }))
  const otherEntries = entries.filter((entry) => !named.has(entry.name))

  return (
    <>
      <TabHeader
        title='General'
        description={
          <>
            Overview of the current state of the ads system. Enable{' '}
            <Link
              href='brave://flags/#ads-internals-verbose-mode'
              target='_blank'
              rel='noopener noreferrer'
            >
              brave://flags/#ads-internals-verbose-mode
            </Link>{' '}
            for additional diagnostics.
          </>
        }
        onRefresh={actions.loadDiagnostics}
      >
        <DeviceIdRow />
      </TabHeader>
      {sections.map((section) => (
        <DiagnosticSection
          key={section.title}
          title={section.title}
          entries={section.entries}
        />
      ))}
      <DiagnosticIdRow />
      <DiagnosticSection title='Other' entries={otherEntries} />
    </>
  )
}
