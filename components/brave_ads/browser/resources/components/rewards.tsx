/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { DiagnosticEntry } from '../lib/app_store'
import {
  isWalletConnected,
  resolveDiagnosticName,
  resolveDiagnosticValue,
} from '../lib/diagnostics'
import { MatchIcon } from './match_icon'
import { TabHeader } from './tab_header'

// There's no way to connect a Brave Rewards wallet on iOS, so the row would
// always read "No" there regardless of Rewards state.
// <if expr="is_ios">
const WALLET_CONNECTION_SUPPORTED = false
// </if>
// <if expr="!is_ios">
const WALLET_CONNECTION_SUPPORTED = true
// </if>

const REWARDS_VALUE_LABELS: Record<
  string,
  Record<string, { label: string; isProblem: boolean }>
> = {
  'Wallet valid': {
    'true': { label: 'Valid', isProblem: false },
    'false': { label: 'Invalid', isProblem: true },
  },
  'Connected': {
    'true': { label: 'Yes', isProblem: false },
    'false': { label: 'No', isProblem: false },
  },
  'Issuers valid': {
    'true': { label: 'Valid', isProblem: false },
    'false': { label: 'Invalid', isProblem: true },
  },
}

const REWARDS_NAME_LABELS: Record<string, string> = {
  'Rewards enabled': 'Enabled',
  'Wallet valid': 'Wallet',
  'Issuers valid': 'Issuers',
}

function displayValue(name: string, rawValue: string) {
  return resolveDiagnosticValue(name, rawValue, REWARDS_VALUE_LABELS)
}

function displayName(name: string) {
  return resolveDiagnosticName(name, REWARDS_NAME_LABELS)
}

// All of these read as the same shape of fact (a prerequisite that's either
// satisfied or not), so they share the same tick/cross treatment rather than
// mixing plain text in with icon rows.
const VALIDITY_ENTRY_NAMES =
  new Set(['Rewards enabled', 'Wallet valid', 'Connected', 'Issuers valid'])

export function Rewards() {
  const actions = useAppActions()
  const rawEntries = useAppState((state) => state.rewardsDiagnosticEntries)
  const rewardsEnabled = useAppState((state) => state.rewardsEnabled)

  React.useEffect(() => {
    actions.loadDiagnostics()
  }, [])

  const walletConnected = isWalletConnected(rawEntries)

  // "Issuers valid" no-ops until the wallet is connected, not just once
  // Rewards is joined; "Invalid" until then isn't worth showing.
  const entries: DiagnosticEntry[] = [
    { name: 'Rewards enabled', value: String(rewardsEnabled) },
    ...(rewardsEnabled
      ? ['Wallet valid', 'Connected', 'Issuers valid']
        .filter((name) =>
          WALLET_CONNECTION_SUPPORTED || name !== 'Connected')
        .filter((name) => walletConnected || name !== 'Issuers valid')
        .map((name) => rawEntries.find((entry) => entry.name === name))
        .filter((entry): entry is DiagnosticEntry => entry !== undefined)
      : []),
  ]

  return (
    <>
      <TabHeader
        title='Rewards'
        description="Brave Rewards account state. This determines whether
          notification ads are served, and whether the user earns rewards
          for viewing notification and new tab page ads."
        onRefresh={actions.loadDiagnostics}
      />
      <div className='content-card'>
        <h4>
          <span className='title'>Rewards</span>
        </h4>
        <section className='key-value-list'>
          {entries.map((entry, index) => {
            const { label, isProblem } = displayValue(entry.name, entry.value)
            const isValidity = VALIDITY_ENTRY_NAMES.has(entry.name)
            return (
              <React.Fragment key={entry.name}>
                <div>
                  <span>{displayName(entry.name)}</span>
                  {isValidity
                    ? <MatchIcon isMatch={entry.value === 'true'} />
                    : (
                      <span className={isProblem ? 'diagnostic-problem' : ''}>
                        {label}
                      </span>
                    )}
                </div>
                {entry.name === 'Rewards enabled' && index < entries.length - 1 && (
                  <hr className='diagnostic-divider' />
                )}
              </React.Fragment>
            )
          })}
        </section>
      </div>
    </>
  )
}
