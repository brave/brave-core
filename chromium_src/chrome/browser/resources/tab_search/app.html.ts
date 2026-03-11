// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Override upstream app.html.ts to render a Brave wrapper component that
// handles tab routing between tab search and Leo AI tab organization.
import {html} from '//resources/lit/v3_0/lit.rollup.js'

import './brave_tab_search_app.js'
import type {TabSearchAppElement} from './app.js'

export function getHtml(this: TabSearchAppElement) {
  return html`
    <brave-tab-search-app .availableHeight="${this.availableHeight_}">
    </brave-tab-search-app>`;
}
