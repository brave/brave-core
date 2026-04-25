// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import type { BraveTabSearchAppElement } from './brave_tab_search_app.js'

export function getHtml(this: BraveTabSearchAppElement) {
  return !this.tabOrganizationEnabled_
    ? html` <tab-search-page available-height="${this.availableHeight}">
      </tab-search-page>`
    : html` <cr-tabs
          .tabNames="${this.tabNames_}"
          .selected="${this.selectedTabIndex_}"
          @selected-changed="${this.onTabSelectedChanged_}"
        >
        </cr-tabs>
        ${this.selectedTabIndex_ === 0
          ? html`
              <tab-search-page available-height="${this.availableHeight}">
              </tab-search-page>
            `
          : html`
              <tab-focus-page available-height="${this.availableHeight}">
              </tab-focus-page>
            `}`
}
