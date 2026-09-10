/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState, useAppActions } from '../lib/app_context'
import { DiagnosticEntry } from '../lib/app_store'
import {
  getDiagnosticValue,
  isWalletConnected,
  resolveDiagnosticName,
} from '../lib/diagnostics'
import { MatchIcon } from './match_icon'
import { TabHeader } from './tab_header'

// Every permission rule reports "true"/"false" for whether it currently
// allows ads; "false" is worth flagging since it's a reason ads aren't being
// served. Every rule here only ever gets evaluated once Rewards is joined.
const PERMISSION_RULE_NAMES = new Set([
  'Catalog permission',
  'Network connection permission',
  'Browser is active permission',
  'Full screen mode permission',
  'Media permission',
  'Do not disturb permission',
  'Issuers permission',
  'Confirmation tokens permission',
  'User activity permission',
  'Command line permission',
  'Can show notifications permission',
])

// "Command line" alone doesn't say what's being checked; this reports whether
// ads-related command-line flags were overridden, which isn't supported
// outside staging.
const PERMISSION_RULE_NAME_LABELS: Record<string, string> = {
  'Catalog permission': 'Has catalog',
  'Network connection permission': 'Has network connection',
  'Browser is active permission': 'Browser is active',
  'Full screen mode permission': 'Full screen mode',
  'Media permission': 'Has media permission',
  'Do not disturb permission': 'Do not disturb',
  'Issuers permission': 'Has issuers',
  'Confirmation tokens permission': 'Has confirmation tokens',
  'User activity permission': 'Has user activity',
  'Command line permission': 'No command-line override',
  'Can show notifications permission': 'Can show notifications',
}

function displayName(name: string) {
  return resolveDiagnosticName(name, PERMISSION_RULE_NAME_LABELS)
}

// Which ad format(s) permission is actually gated for, derived from the
// `HasPermission()` call sites in serving/permission_rules/*, not just the
// rule's name. `PermissionRulesBase` (issuers/confirmation tokens/command
// line) is a shared prerequisite checked by every format's `HasPermission()`.
// Every rule here requires Rewards to be joined, so Search Result ads' own
// "not joined -> skip" bypass (`UserHasJoinedBraveRewards()` in
// search_result_ad_permission_rules.cc) never applies; only New Tab Page ads
// has a bypass that can still trigger despite Rewards being joined, since it
// additionally requires a connected wallet
// (`UserHasJoinedBraveRewardsAndConnectedWallet()` in settings.cc).
const PERMISSION_RULE_AD_FORMATS: Record<string, string[]> = {
  'Has catalog': ['Notification ads'],
  'Has network connection': ['Notification ads'],
  'Browser is active': ['Notification ads'],
  'Full screen mode': ['Notification ads'],
  'Has media permission': ['Notification ads'],
  'Do not disturb': ['Notification ads'],
  'Has issuers': ['Notification ads', 'New tab page ads', 'Search result ads'],
  'Has confirmation tokens':
    ['Notification ads', 'New tab page ads', 'Search result ads'],
  'Has user activity': ['Notification ads', 'New tab page ads'],
  'No command-line override':
    ['Notification ads', 'New tab page ads', 'Search result ads'],
  'Can show notifications': ['Notification ads'],
}

// New Tab Page ads skips every `PermissionRulesBase` rule (and its own
// `HasUserActivityPermission()` check) entirely unless a wallet is connected,
// so these rules no-op for that format until then.
const WALLET_GATED_NTP_RULE_NAMES = new Set([
  'Has issuers',
  'Has confirmation tokens',
  'Has user activity',
  'No command-line override',
])

// Maps an ad format's label here to the diagnostic entry that says whether
// the device is actually opted into it.
const AD_FORMAT_ENABLED_ENTRY_NAMES: Record<string, string> = {
  'Notification ads': 'Notification ads enabled',
  'New tab page ads': 'New tab page ads shown',
  'Search result ads': 'Sponsored ads enabled',
}

function getAdFormatsLabel(
  name: string,
  walletConnected: boolean,
  disabledFormats: Set<string>,
) {
  const formats = PERMISSION_RULE_AD_FORMATS[name]
  if (!formats) {
    return 'Unknown'
  }

  // No-ops for New Tab Page ads until a wallet is connected, and for any
  // format the device isn't opted into, so those are left out of the list
  // entirely rather than listed with a caveat.
  const applicableFormats = formats.filter((format) =>
    (walletConnected || format !== 'New tab page ads' ||
      !WALLET_GATED_NTP_RULE_NAMES.has(name)) &&
    !disabledFormats.has(format))
  return applicableFormats.length === 0 ? 'None' : applicableFormats.join(', ')
}

export function PermissionRules() {
  const actions = useAppActions()
  const rawEntries = useAppState((state) => state.permissionRulesDiagnosticEntries)
  const rewardsEntries = useAppState((state) => state.rewardsDiagnosticEntries)
  const diagnosticEntries = useAppState((state) => state.diagnosticEntries)

  React.useEffect(() => {
    actions.loadDiagnostics()
  }, [])

  const entries = Array.from(PERMISSION_RULE_NAMES)
    .map((name) => rawEntries.find((entry) => entry.name === name))
    .filter((entry): entry is DiagnosticEntry => entry !== undefined)

  const walletConnected = isWalletConnected(rewardsEntries)

  const disabledFormats = new Set(
    Object.entries(AD_FORMAT_ENABLED_ENTRY_NAMES)
      .filter(([, entryName]) =>
        getDiagnosticValue(diagnosticEntries, entryName) === 'false')
      .map(([format]) => format),
  )

  return (
    <>
      <TabHeader
        title='Permission rules'
        description='Whether each serving rule currently allows ads, and
          which ad formats it applies to.'
        onRefresh={actions.loadDiagnostics}
      />
      <div className='content-card'>
        <section>
          <table>
            <thead>
              <tr>
                <th className='rule-column'>Rule</th>
                <th>Ad formats</th>
                <th className='status-column'>Status</th>
              </tr>
            </thead>
            <tbody>
              {entries.map((entry) => {
                const name = displayName(entry.name)
                return (
                  <tr key={entry.name}>
                    <td>{name}</td>
                    <td>
                      {getAdFormatsLabel(name, walletConnected, disabledFormats)}
                    </td>
                    <td>
                      <MatchIcon isMatch={entry.value === 'true'} />
                    </td>
                  </tr>
                )
              })}
            </tbody>
          </table>
        </section>
      </div>
    </>
  )
}
