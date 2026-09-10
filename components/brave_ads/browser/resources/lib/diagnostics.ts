/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { DiagnosticEntry } from './app_store'

// Every tab looks up its own diagnostic entries by name from a shared flat
// array; centralized so a future change to how entries are matched only
// needs to happen once.
export function getDiagnosticValue(
  entries: DiagnosticEntry[],
  name: string,
): string | undefined {
  return entries.find((entry) => entry.name === name)?.value
}

export function isWalletConnected(entries: DiagnosticEntry[]) {
  return getDiagnosticValue(entries, 'Connected') === 'true'
}

// Several tabs shorten a backend entry's raw name to a friendlier row label,
// via their own `name -> label` map; centralized since the lookup-with-
// fallback shape is identical everywhere it's used.
export function resolveDiagnosticName(
  name: string,
  nameLabels: Record<string, string>,
): string {
  return nameLabels[name] ?? name
}

export interface DiagnosticValueLabel {
  label: string
  isProblem: boolean
  isSuccess?: boolean
}

// Several tabs also translate a backend entry's raw "true"/"false" value
// into a human-readable label (optionally flagging it as a problem or
// success state), via their own `name -> value -> label` map; centralized
// for the same reason as `resolveDiagnosticName` above.
export function resolveDiagnosticValue(
  name: string,
  value: string,
  valueLabels: Record<string, Record<string, DiagnosticValueLabel>>,
): DiagnosticValueLabel {
  return valueLabels[name]?.[value] ?? { label: value, isProblem: false }
}
