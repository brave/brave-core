// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// The fragment of a conversation's URL that names an entry for the
// conversation to scroll to. The page and the frame showing the entries both
// carry it.
//
// A fragment names the same entry again for each use, so that using it again
// scrolls again even though the entry is the same.

export interface EntryTarget {
  entryUuid: string
  // Differs for each use of the fragment.
  nonce: string
}

export function makeEntryFragment(entryUuid: string): string {
  return `#${new URLSearchParams({ entry: entryUuid, n: `${Date.now()}` })}`
}

export function parseEntryFragment(hash: string): EntryTarget | undefined {
  const params = new URLSearchParams(
    hash.startsWith('#') ? hash.slice(1) : hash,
  )
  const entryUuid = params.get('entry')
  if (!entryUuid) {
    return undefined
  }
  return { entryUuid, nonce: params.get('n') ?? '' }
}
