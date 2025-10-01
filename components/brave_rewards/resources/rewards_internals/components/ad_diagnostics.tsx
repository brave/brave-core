/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_model_context'

import { style } from './ad_diagnostics.style'

export function AdDiagnostics() {
  const actions = useAppActions()
  const entries = useAppState((state) => state.adDiagnosticEntries)
  const diagnosticId = useAppState((state) => state.adDiagnosticId)

  return (
    <div
      className='content-card'
      data-css-scope={style.scope}
    >
      <h4>Ad Diagnostics</h4>
      <section className='key-value-list'>
        {entries.map((entry) => (
          <div key={entry.name}>
            <span>{entry.name}</span>
            <span>{entry.value}</span>
          </div>
        ))}
        <div>
          <span>Diagnostic ID:</span>
          <span>
            <input
              value={diagnosticId}
              maxLength={36}
              autoComplete='off'
              spellCheck={false}
              onChange={(event) => {
                actions.setAdDiagnosticId(event.target.value)
              }}
            />
          </span>
        </div>
      </section>
    </div>
  )
}
