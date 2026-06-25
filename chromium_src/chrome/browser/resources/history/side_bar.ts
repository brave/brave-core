// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_toggle/cr_toggle.js'

import {injectStyle} from '//resources/brave/lit_overriding.js'
import {loadTimeData} from '//resources/js/load_time_data.js'
import {css} from '//resources/lit/v3_0/lit.rollup.js'
import type {PropertyValues} from '//resources/lit/v3_0/lit.rollup.js'

import {
  HistorySideBarElement as HistorySideBarElementChromium,
} from './side_bar-chromium.js'

// <if expr="enable_local_ai">
import {
  getBraveHistoryEmbeddingsBrowserProxy,
} from './brave_history_embeddings_browser_proxy.js'
// </if>

injectStyle(HistorySideBarElementChromium, css`
  .cr-nav-menu-item {
    min-height: 20px !important;
    border-end-end-radius: 0px !important;
    border-start-end-radius: 0px !important;
    box-sizing: content-box !important;
  }
  .cr-nav-menu-item:hover {
    background: transparent !important;
  }
  .cr-nav-menu-item[selected] {
    --iron-icon-fill-color: var(--cr-link-color) !important;
    color: var(--cr-link-color) !important;
    background: transparent !important;
  }
  .cr-nav-menu-item cr-icon {
    display: none !important;
  }
  .cr-nav-menu-item cr-ripple {
    display: none !important;
  }
  #brave-history-embeddings-toggle {
    align-items: center;
    border-top: 1px solid var(--cr-separator-color);
    display: flex;
    gap: 12px;
    margin-top: 8px;
    padding: 12px 16px 8px;
  }
  #brave-history-embeddings-toggle .text {
    flex: 1;
  }
  #brave-history-embeddings-toggle .label {
    font-weight: 500;
  }
  #brave-history-embeddings-toggle .description {
    color: var(--cr-secondary-text-color);
    font-size: 12px;
    margin-top: 2px;
  }
`)

// Declaration-merge the Brave-only reactive properties onto the upstream
// class type so the lit_mangler-injected template (typed with
// `this: HistorySideBarElement` via the upstream side_bar.html.ts import)
// type-checks.
declare module './side_bar-chromium.js' {
  interface HistorySideBarElement {
    braveHistoryEmbeddingsFeatureEnabled: boolean
    braveHistoryEmbeddingsEnabled: boolean
    onBraveHistoryEmbeddingsToggleChange(e: CustomEvent<boolean>): void
  }
}

class HistorySideBarElement extends HistorySideBarElementChromium {
  static override get properties() {
    return {
      ...super.properties,
      braveHistoryEmbeddingsFeatureEnabled: {type: Boolean},
      braveHistoryEmbeddingsEnabled: {type: Boolean},
    }
  }

  override accessor braveHistoryEmbeddingsFeatureEnabled: boolean =
      loadTimeData.getBoolean('isHistoryEmbeddingsFeatureEnabled')
  override accessor braveHistoryEmbeddingsEnabled: boolean =
      loadTimeData.getBoolean('enableHistoryEmbeddings')

  // Re-read the toggle state from loadTimeData on every render cycle. The
  // single Mojo subscription in app.ts updates loadTimeData and then calls
  // requestUpdate() on this element, the toolbar, and the list — keeping all
  // consumers on the same refresh mechanism.
  override willUpdate(changedProperties: PropertyValues<this>) {
    super.willUpdate(changedProperties)
    this.braveHistoryEmbeddingsEnabled =
        loadTimeData.getBoolean('enableHistoryEmbeddings')
  }

  // <if expr="enable_local_ai">
  override onBraveHistoryEmbeddingsToggleChange(e: CustomEvent<boolean>) {
    getBraveHistoryEmbeddingsBrowserProxy().pageHandler.setEnabled(e.detail)
  }
  // </if>
}

export {HistorySideBarElement}
export * from './side_bar-chromium.js'

// Register the Brave subclass instead of the upstream class. The matching
// `customElements.define` in upstream side_bar.ts is patched out (see
// patches/chrome-browser-resources-history-side_bar.ts.patch).
customElements.define(HistorySideBarElement.is, HistorySideBarElement)
