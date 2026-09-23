// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Inject a Semantic History Search toggle above the side bar's #spacer, plus a
// "Relaunch" button on its own row below it (shown once the toggle changes).
// The referenced properties and handlers live on the Brave
// HistorySideBarElement subclass in side_bar.ts; the augmented interface lets
// `this.<prop>` accesses type-check here.
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
       </div>
       <div id="brave-history-embeddings-restart"
            ?hidden="\${!this.braveHistoryEmbeddingsFeatureEnabled ||
                       !this.braveHistoryEmbeddingsNeedsRestart}">
         <cr-button @click="\${this.onBraveHistoryEmbeddingsRelaunchClick}">
           $i18n{braveHistoryEmbeddingsRelaunchButtonLabel}
         </cr-button>
       </div>`,
    )
  },
  (literal) => literal.text.includes('id="spacer"'),
)
