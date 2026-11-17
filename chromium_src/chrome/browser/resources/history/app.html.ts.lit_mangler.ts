// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Puts Brave's semantic history search results above the history list. The
// element is imported by the companion app.ts override. The check is at
// runtime because build-time conditionals in mangled output are never
// evaluated, and reads loadTimeData because the template first renders before
// the rest of app.ts runs. The query is an attribute binding: without local AI the element isn't
// built, and the history lint can only check a property binding against an
// element it knows.
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
      `\${loadTimeData.getBoolean('enableBraveHistoryEmbeddings') ? html\`
        <brave-history-embeddings class="history-cards"
            search-query="\${this.queryState_.searchTerm}">
        </brave-history-embeddings>
      \` : ''}`,
    )
  },
  (literal) => literal.text.includes('id="tabsContent"'),
)
