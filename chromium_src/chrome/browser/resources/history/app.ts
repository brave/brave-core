// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {injectStyle} from '//resources/brave/lit_overriding.js'
import {css} from '//resources/lit/v3_0/lit.rollup.js'

import {HistoryAppElement} from './app-chromium.js'

// <if expr="enable_local_ai">
import {loadTimeData} from '//resources/js/load_time_data.js'
import {
  getBraveHistoryEmbeddingsBrowserProxy,
} from './brave_history_embeddings_browser_proxy.js'

// Subscribe directly to the Mojo `onEnabledChanged` notification so the
// chrome://history UI reacts even when the side bar isn't rendered. Update
// each history-app's reactive `enableHistoryEmbeddings_` accessor (Lit
// re-renders the app shell), refresh loadTimeData so any code that reads
// `enableHistoryEmbeddings` inline picks up the new value, and force a
// re-render of the toolbar/list whose `compute*_` methods cache the value.
getBraveHistoryEmbeddingsBrowserProxy()
    .callbackRouter.onEnabledChanged.addListener((enabled: boolean) => {
      loadTimeData.overrideValues({enableHistoryEmbeddings: enabled})
      for (const app of document.querySelectorAll('history-app')) {
        ;(app as unknown as {enableHistoryEmbeddings_: boolean})
            .enableHistoryEmbeddings_ = enabled
      }
      const root = document.querySelector('history-app')?.shadowRoot
      if (!root) {
        return
      }
      for (const el of root.querySelectorAll(
               'history-side-bar, history-toolbar, history-list')) {
        ;(el as unknown as {requestUpdate?: () => void}).requestUpdate?.()
      }
    })
// </if>

injectStyle(HistoryAppElement, css`
        #tabs-content, #tabs-content>* {
            height: 100%;
        }
    </style>
`)

export * from './app-chromium.js'
