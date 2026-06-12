// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { TabSearchPageElement } from './tab_search_page-chromium.js'

import type { Tab } from './tab_search.mojom-webui.js'
import type { TabData } from './tab_data.js'
import {
  BraveTabSearchApiProxy,
  BraveTabSearchApiProxyImpl,
} from './tab_search_api_proxy.js'

// Augments the substring-filtered open-tab list with on-device semantic
// matches from `searchTabsByContent`. A monotonic token discards stale
// responses when the user types faster than the on-device ranker completes.
class BraveTabSearchPageElement extends TabSearchPageElement {
  private semanticSearchToken_: number = 0

  protected override async processFilteredOpenTabs_(
      tabs: TabData[]): Promise<TabData[]> {
    const searchText = (this as unknown as {searchText_: string}).searchText_
    if (!searchText) {
      return tabs
    }
    const openTabs = (this as unknown as {openTabs_: TabData[]}).openTabs_
    const token = ++this.semanticSearchToken_
    try {
      const proxy =
          BraveTabSearchApiProxyImpl.getInstance() as BraveTabSearchApiProxy
      const {tabIds} = await proxy.searchTabsByContent(searchText)
      if (token !== this.semanticSearchToken_) {
        return tabs
      }
      const present = new Set<number>(tabs.map(d => (d.tab as Tab).tabId))
      const semanticMatches = tabIds
          .map(id => openTabs.find(d => (d.tab as Tab).tabId === id))
          .filter((d): d is TabData =>
              !!d && !present.has((d.tab as Tab).tabId))
      return tabs.concat(semanticMatches)
    } catch {
      return tabs
    }
  }
}

customElements.define(
    BraveTabSearchPageElement.is, BraveTabSearchPageElement)

export * from './tab_search_page-chromium.js'
