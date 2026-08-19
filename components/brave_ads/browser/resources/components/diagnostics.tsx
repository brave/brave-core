/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'

const DIAGNOSTIC_ID_MAX_LENGTH = 36

export function Diagnostics() {
  const actions = useAppActions()
  const entries = useAppState((state) => state.diagnosticEntries)
  const diagnosticId = useAppState((state) => state.diagnosticId)

  return (
    <>
      <div className='content-card'>
        <h4>
          <span className='title'>Diagnostics</span>
        </h4>
        <section className='key-value-list'>
          {entries.map((entry) => (
            <div key={entry.name}>
              <span>{entry.name}</span>
              <span>{entry.value}</span>
            </div>
          ))}
        </section>
      </div>

      <div className='content-card'>
        <section className='key-value-list'>
          <div>
            <span>Diagnostic ID</span>
            <span>
              <input
                value={diagnosticId}
                maxLength={DIAGNOSTIC_ID_MAX_LENGTH}
                autoComplete='off'
                spellCheck={false}
                onChange={(event) => {
                  actions.setDiagnosticId(event.target.value)
                }}
              />
            </span>
          </div>
        </section>
      </div>
    </>
  )
}
