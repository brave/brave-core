// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Puts Brave's semantic history search results above the history list. The
// element is imported by the companion app.ts override.
mangle(
  (element) => {
    const tabsContent = element.querySelector('#tabsContent')
    if (!tabsContent) {
      throw new Error(
        '[Brave History] Could not find #tabsContent. Has upstream changed?',
      )
    }
    tabsContent.insertAdjacentHTML(
      'beforebegin',
      `<brave-history-embeddings class="history-cards"
          .searchQuery="\${this.queryState_.searchTerm}">
      </brave-history-embeddings>`,
    )
  },
  (literal) => literal.text.includes('id="tabsContent"'),
)
