// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { RegisterPolymerPrototypeModification, RegisterPolymerTemplateModifications, RegisterStyleOverride } from 'chrome://resources/brave/polymer_overriding.js';
import { loadTimeData } from 'chrome://resources/js/load_time_data.js';
import { html } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { BraveTabSearch } from '../brave_tab_search.mojom-webui.js';
import { fuzzySearch } from '../fuzzy_search.js';
import { ItemData } from '../tab_data.js';
import { TitleItem } from '../title_item.js';
import { HISTORY_ENTRY_TAB_ITEM_TYPE, HistoryEntryData } from './tab_data.js';
import { NO_SELECTION } from '../infinite_list.js';

RegisterStyleOverride('tab-search-page', html`
  <style>
    .history-entry {
      display: flex;
      flex-direction: column;
      gap: 2px;
      overflow: hidden;
      outline: none;
      text-overflow: ellipsis;
      white-space: nowrap;
      align-items: start;
    }

    .history-entry .title {
      color: var(--cr-primary-text-color);
      font-size: var(--mwb-primary-text-font-size);
      font-weight: var(--mwb-primary-text-font-weight);
    }

    .history-entry .url {
      color: var(--cr-secondary-text-color);
      font-size: var(--mwb-secondary-text-font-size);
      font-weight: var(--mwb-secondary-text-font-weight);
    }
  </style>
`)


RegisterPolymerTemplateModifications({
  'tab-search-page': template => {
    const infiniteList = template.querySelector('#tabsList')

    const historyEntryTemplate = html`
      <div id="[[item.historyEntry.url]]" class="mwb-list-item history-entry"
        data="[[item]]" index="[[index]]"
        on-click="onItemClick_" on-close="onItemClose_"
        on-focus="onItemFocus_" on-keydown="onItemKeyDown_" role="option"
        tabindex="0">
        <span class="title">[[item.historyEntry.title]]</span>
        <span class="url">[[item.historyEntry.url.url]]</span>
      </div>
    `
    historyEntryTemplate.setAttribute('data-type', 'HistoryEntryData')
    historyEntryTemplate.setAttribute('data-selectable', '')

    infiniteList.appendChild(historyEntryTemplate)
  }
})

let historyData: HistoryEntryData[] = []
if (loadTimeData.getBoolean('tabSearchHistory')) {
  BraveTabSearch.getRemote().getHistoryEntries().then(({ history }) => {
    historyData = history.map((e) => new HistoryEntryData(e))
  })
}

const historyTitleItem = new TitleItem("History", true, true)
RegisterPolymerPrototypeModification({
  'tab-search-page': prototype => {
    const internalUpdateFilteredTabs = prototype.updateFilteredTabs_

    prototype.updateFilteredTabs_ = function () {
      let selectedIndex = this.getSelectedIndex();

      const historyItems = fuzzySearch(this.searchText_, historyData, this.fuzzySearchOptions_)

      internalUpdateFilteredTabs.apply(this)

      if (historyItems.length) {
        const result = [...this.filteredItems_, historyTitleItem]
        if (historyTitleItem.expanded) result.push(...historyItems)
        this.filteredItems_ = result
      }

      this.searchResultText_ = this.getA11ySearchResultText_();

      // If there was no previously selected index, set the selected index to be
      // the tab index specified for initial selection; else retain the currently
      // selected index. If the list shrunk above the selected index, select the
      // last index in the list. If there are no matching results, set the
      // selected index value to none.
      const tabsList = this.$.tabsList;
      if (selectedIndex === NO_SELECTION) {
        selectedIndex = this.initiallySelectedTabIndex_;
      }
      tabsList.selected =
        Math.min(Math.max(selectedIndex, 0), this.selectableItemCount_() - 1);
    }

    const internalTabItemAction = prototype.tabItemAction_
    prototype.tabItemAction_ = function (itemData: ItemData, tabIndex: number) {
      if (itemData.type === HISTORY_ENTRY_TAB_ITEM_TYPE as any) {
        BraveTabSearch.getRemote().openUrl((itemData as HistoryEntryData).historyEntry.url)
        return
      }

      internalTabItemAction.call(this, itemData, tabIndex)
    }

    const internalReady = prototype.ready
    prototype.ready = function () {
      internalReady.call(this)

      // Search the title of history entries.
      this.fuzzySearchOptions_.keys.push({
        name: 'historyEntry.title',
        weight: loadTimeData.getValue('searchTitleWeight')
      })
    }

    const internalOnTitleExpandChanged = prototype.onTitleExpandChanged_
    prototype.onTitleExpandChanged_ = function(e: any) {
      if (e.detail.value === e.model.item.expanded) return
      internalOnTitleExpandChanged.call(this, e)
    }
  }
})
