/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Keep existing built-in preferences, and distinguish custom searches that
// share a host by their unique keyword.
export function getSearchEngineId(engine: {
  prepopulateId: bigint
  host: string
  keyword: string
}): string {
  return engine.prepopulateId > 0n ? engine.host : `keyword:${engine.keyword}`
}
