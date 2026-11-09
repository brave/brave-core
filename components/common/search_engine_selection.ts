/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getSearchEngineId } from './search_engine_id'

type Engine = Parameters<typeof getSearchEngineId>[0]

// Discard unavailable selections and always offer an available fallback.
export function getEnabledSearchEngineIds(
  engines: Engine[],
  configured: Set<string>,
  defaultEngine: string,
): Set<string> {
  const enabled = new Set(
    engines.map(getSearchEngineId).filter((id) => configured.has(id)),
  )
  if (!enabled.size) {
    const fallback =
      engines.find((engine) => getSearchEngineId(engine) === defaultEngine)
      ?? engines.find(
        (engine) =>
          engine.prepopulateId > 0n && engine.host === 'search.brave.com',
      )
      ?? engines[0]
    if (fallback) enabled.add(getSearchEngineId(fallback))
  }
  return enabled
}

export function getSelectedSearchEngineId(
  engines: Engine[],
  configured: Set<string>,
  lastUsed: string,
  defaultEngine: string,
): string {
  const enabled = getEnabledSearchEngineIds(engines, configured, defaultEngine)
  if (enabled.has(lastUsed)) return lastUsed
  if (enabled.has(defaultEngine)) return defaultEngine
  return enabled.values().next().value ?? defaultEngine
}
