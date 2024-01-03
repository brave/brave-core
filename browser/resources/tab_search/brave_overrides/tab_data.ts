// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {TabItemType, ItemData} from '../tab_data.js'
import {HistoryEntry} from '../brave_tab_search.mojom-webui.js'

export const HISTORY_ENTRY_TAB_ITEM_TYPE = -1;
(TabItemType as any).HISTORY_ENTRY = HISTORY_ENTRY_TAB_ITEM_TYPE;

export class HistoryEntryData extends ItemData {
  historyEntry: HistoryEntry;

  get hostname() {
    return this.historyEntry.url.url
  }

  constructor(historyEntry: HistoryEntry) {
    super()

    this.historyEntry = historyEntry
    this.type = HISTORY_ENTRY_TAB_ITEM_TYPE as any
  }
}
