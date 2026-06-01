// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Inject a Semantic History Search toggle above the side bar's #spacer.
// The referenced properties and handler live on the Brave HistorySideBarElement
// subclass declared in side_bar.ts; the augmented HistorySideBarElement
// interface lets `this.<prop>` accesses type-check here.
mangle(
  (element) => {
    const spacer = element.querySelector('#spacer')
    if (!spacer) {
      throw new Error(
        '[Brave History] Could not find #spacer. Has upstream changed?',
      )
    }
    spacer.insertAdjacentHTML(
      'beforebegin',
      `<div id="brave-history-embeddings-toggle"
            ?hidden="\${!this.braveHistoryEmbeddingsFeatureEnabled}">
         <div class="text">
           <div class="label">$i18n{braveHistoryEmbeddingsToggleLabel}</div>
           <div class="description">
             $i18n{braveHistoryEmbeddingsToggleDescription}
           </div>
         </div>
         <cr-toggle
             ?checked="\${this.braveHistoryEmbeddingsEnabled}"
             @change="\${this.onBraveHistoryEmbeddingsToggleChange}">
         </cr-toggle>
       </div>`,
    )
  },
  (literal) => literal.text.includes('id="spacer"'),
)
