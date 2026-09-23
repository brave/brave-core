/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { getDiagnosticValue } from '../lib/diagnostics'
import { TabHeader } from './tab_header'

export function ConfirmationTokens() {
  const actions = useAppActions()
  const rawEntries =
    useAppState((state) => state.confirmationTokensDiagnosticEntries)

  React.useEffect(() => {
    actions.loadDiagnostics()
  }, [])

  const remaining =
    getDiagnosticValue(rawEntries, 'Confirmation tokens remaining') ?? '0'

  return (
    <>
      <TabHeader
        title='Confirmation Tokens'
        description='Tokens used to confirm ad events.'
        onRefresh={actions.loadDiagnostics}
      />
      <div className='content-card'>
        <section className='key-value-list'>
          <div>
            <span>Remaining tokens</span>
            <span>{remaining}</span>
          </div>
        </section>
      </div>
    </>
  )
}
