// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../api/psst_internals_api'

export const levelNames = new Map<Mojom.PsstLogLevel, string>([
  [Mojom.PsstLogLevel.kDebug, 'debug'],
  [Mojom.PsstLogLevel.kInfo, 'info'],
  [Mojom.PsstLogLevel.kWarn, 'warn'],
  [Mojom.PsstLogLevel.kError, 'error'],
])

export const levelIcons = new Map<Mojom.PsstLogLevel, string>([
  [Mojom.PsstLogLevel.kDebug, 'code'],
  [Mojom.PsstLogLevel.kInfo, 'info-filled'],
  [Mojom.PsstLogLevel.kWarn, 'warning-triangle-filled'],
  [Mojom.PsstLogLevel.kError, 'warning-circle-filled'],
])

export const sourceNames = new Map<Mojom.PsstLogSource, string>([
  [Mojom.PsstLogSource.kNative, 'native'],
  [Mojom.PsstLogSource.kScript, 'script'],
])

export function formatTime(timestampMs: number) {
  const date = new Date(timestampMs)
  const time = date.toLocaleTimeString(undefined, { hour12: false })
  return `${time}.${String(date.getMilliseconds()).padStart(3, '0')}`
}

export function formatTabId(tabId: number | undefined) {
  return tabId === null || tabId === undefined ? '—' : String(tabId);
}

// Tab-separated so the result can be pasted straight into a bug report or a
// spreadsheet.
export function formatEntriesForClipboard(entries: Mojom.PsstLogEntry[]) {
  return entries
    .map((entry) =>
      [
        new Date(entry.timestampMs).toISOString(),
        levelNames.get(entry.level) ?? '',
        sourceNames.get(entry.source) ?? '',
        formatTabId(entry.tabId),
        entry.site,
        entry.message,
        entry.data ?? '',
      ].join('\t'),
    )
    .join('\n')
}
