// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { search as upstreamSearch } from './search-chromium.js'
import type { SearchOptions } from './search-chromium.js'
import type { ItemData, SplitViewData, TabData } from './tab_data.js'
import { TabData as TabDataClass } from './tab_data.js'

export type { OptionKeyObject, SearchOptions } from './search-chromium.js'

// Lets BraveTabSearchPageElement inject semantic matches into the open-tab
// list without patching upstream's `updateFilteredTabs_`.
// Only calls passing the records that match `getOpenTabsRecords()` are
// augmented, so the media-tabs and recently-closed `search()` calls are
// unaffected. Upstream replaces `openTabs_` with a fresh array on each
// profile-data update, so we resolve the reference lazily via a getter
// rather than caching it at registration time.
interface SemanticOpenTabsContext {
  getOpenTabsRecords(): ReadonlyArray<TabData | SplitViewData>
  getSemanticTabIds(query: string): ReadonlySet<number> | undefined
}

let context: SemanticOpenTabsContext | null = null

export function setSemanticOpenTabsContext(
    ctx: SemanticOpenTabsContext | null): void {
  context = ctx
}

export async function search<T extends ItemData>(
    input: string, records: T[], options: SearchOptions): Promise<T[]> {
  const result = await upstreamSearch(input, records, options)
  if (!input || !context ||
      (records as ReadonlyArray<unknown>) !== context.getOpenTabsRecords()) {
    return result
  }
  const tabIds = context.getSemanticTabIds(input)
  if (!tabIds || tabIds.size === 0) {
    return result
  }
  const present = new Set<number>()
  for (const item of result) {
    if (item instanceof TabDataClass) {
      present.add(item.tab.tabId)
    }
  }
  const extras: T[] = []
  for (const item of records) {
    if (!(item instanceof TabDataClass)) {
      continue
    }
    const tabId = item.tab.tabId
    if (tabIds.has(tabId) && !present.has(tabId)) {
      extras.push(item)
    }
  }
  return result.concat(extras)
}
