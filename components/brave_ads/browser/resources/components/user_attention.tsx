/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { splitTrailingParenthetical } from '../lib/format_time'
import { TabHeader } from './tab_header'

// The backend only tracks this in memory for the current session, so "Never"
// doesn't mean it's truly never happened, just not yet observed since this
// browser session started.
function displayValue(rawValue: string) {
  return rawValue === 'Never' ? 'Not yet this session' : rawValue
}

export function UserAttention() {
  const actions = useAppActions()
  const rawEntries = useAppState((state) => state.diagnosticEntries)

  React.useEffect(() => {
    actions.loadDiagnostics()
  }, [])

  const lastUnidleAt =
    rawEntries.find((entry) => entry.name === 'Last unidle time')?.value ??
      'Never'
  const lastUnidleAtSplit = splitTrailingParenthetical(displayValue(lastUnidleAt))

  return (
    <>
      <TabHeader
        title='User attention'
        description='When the user was last observed to be active.'
        onRefresh={actions.loadDiagnostics}
      />

      <div className='content-card'>
        <section className='key-value-list'>
          <div>
            <span>Last unidle time</span>
            <span>
              <span
                className={lastUnidleAt === 'Never' ? 'diagnostic-muted' : ''}
              >
                {lastUnidleAtSplit.main}
              </span>{' '}
              {lastUnidleAtSplit.parenthetical && (
                <span className='diagnostic-muted'>
                  {lastUnidleAtSplit.parenthetical}
                </span>
              )}
            </span>
          </div>
        </section>
      </div>
    </>
  )
}
