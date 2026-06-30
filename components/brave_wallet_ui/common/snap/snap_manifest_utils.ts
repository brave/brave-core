// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../constants/types'

type SnapManifest = BraveWallet.SnapManifest

const SNAP_MANIFEST_PERMISSION_FIELDS: ReadonlyArray<
  [keyof SnapManifest, string]
> = [
  ['networkAccess', 'endowment:network-access'],
  ['rpc', 'endowment:rpc'],
  ['ethereumProvider', 'endowment:ethereum-provider'],
  ['cronjob', 'endowment:cronjob'],
  ['transactionInsight', 'endowment:transaction-insight'],
  ['signatureInsight', 'endowment:signature-insight'],
  ['keyring', 'endowment:keyring'],
  ['nameLookup', 'endowment:name-lookup'],
  ['pageHome', 'endowment:page-home'],
  ['lifecycleHooks', 'endowment:lifecycle-hooks'],
  ['webassembly', 'endowment:webassembly'],
  ['bip44Entropy', 'snap_getBip44Entropy'],
  ['bip32Entropy', 'snap_getBip32Entropy'],
  ['getEntropy', 'snap_getEntropy'],
  ['dialog', 'snap_dialog'],
  ['notify', 'snap_notify'],
  ['manageState', 'snap_manageState'],
]

export function getSnapPermissionNames(
  manifest: SnapManifest | null | undefined,
): string[] {
  if (!manifest) {
    return []
  }
  return SNAP_MANIFEST_PERMISSION_FIELDS.filter(
    ([field]) => manifest[field] != null,
  ).map(([, name]) => name)
}

export function getEndowmentPermissionNames(
  manifest: SnapManifest | null | undefined,
): string[] {
  return getSnapPermissionNames(manifest).filter((name) =>
    name.startsWith('endowment:'),
  )
}

export function getSnapManifestForDisplay(
  manifest: SnapManifest | null | undefined,
): Record<string, unknown> | null {
  if (!manifest) {
    return null
  }

  const result: Record<string, unknown> = {}

  if (manifest.proposedName) {
    result.proposedName = manifest.proposedName
  }
  if (manifest.description) {
    result.description = manifest.description
  }

  for (const [field] of SNAP_MANIFEST_PERMISSION_FIELDS) {
    const value = manifest[field]
    if (value != null) {
      result[field] = value
    }
  }

  return Object.keys(result).length > 0 ? result : null
}
