/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { DiagnosticEntry } from '../lib/app_store'
import { MatchIcon } from './match_icon'
import { TabHeader } from './tab_header'

const STORAGE_SECTIONS: Array<{
  title: string
  names: string[]
}> = [
  {
    title: 'Database',
    names: ['Schema version', 'Last migration failure reason'],
  },
]

function displayValue(name: string, rawValue: string) {
  if (name === 'Last migration failure reason') {
    return rawValue === 'None'
      ? { label: 'No failures', isProblem: false, isMuted: true }
      : { label: rawValue, isProblem: true, isMuted: false }
  }
  return { label: rawValue, isProblem: false, isMuted: false }
}

function sectionStatus(entries: DiagnosticEntry[]) {
  const isProblem = entries.some(
    (entry) => displayValue(entry.name, entry.value).isProblem,
  )
  return { isProblem }
}

function StorageSection(
  { title, entries }: { title: string; entries: DiagnosticEntry[] },
) {
  if (entries.length === 0) {
    return null
  }

  const status = sectionStatus(entries)

  return (
    <div className='content-card'>
      <h4>
        <span className='title'>{title}</span>
      </h4>
      <section className='key-value-list'>
        <div>
          <span>Status</span>
          <MatchIcon isMatch={!status.isProblem} />
        </div>
        {entries.map((entry) => {
          const { label, isProblem, isMuted } =
            displayValue(entry.name, entry.value)
          return (
            <div key={entry.name}>
              <span>{entry.name}</span>
              <span
                className={
                  isProblem
                    ? 'diagnostic-problem'
                    : isMuted ? 'diagnostic-muted' : ''
                }
              >
                {label}
              </span>
            </div>
          )
        })}
      </section>
    </div>
  )
}

export function Storage() {
  const actions = useAppActions()
  const rawEntries = useAppState((state) => state.storageDiagnosticEntries)

  React.useEffect(() => {
    actions.loadDiagnostics()
  }, [])

  const sections = STORAGE_SECTIONS.map((section) => ({
    title: section.title,
    entries: section.names
      .map((name) => rawEntries.find((entry) => entry.name === name))
      .filter((entry): entry is DiagnosticEntry => entry !== undefined),
  }))

  return (
    <>
      <TabHeader
        title='Storage'
        description='On-disk state for locally persisted data.'
        onRefresh={actions.loadDiagnostics}
      />

      {sections.map((section) => (
        <StorageSection
          key={section.title}
          title={section.title}
          entries={section.entries}
        />
      ))}
    </>
  )
}
