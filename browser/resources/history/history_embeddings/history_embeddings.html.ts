// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import type { BraveHistoryEmbeddingsElement } from './history_embeddings.js'

export function getHtml(this: BraveHistoryEmbeddingsElement) {
  // clang-format off
  return html`<!--_html_template_start_-->
    ${this.enabled_
      ? html`
          <div id="disclaimer">
            ${this.i18n('braveHistoryEmbeddingsDisclaimer')}
          </div>
        `
      : ''}
    ${this.showResults_()
      ? html`
          <div class="card">
            <h2 class="heading results-heading">
              <cr-icon
                icon="${this.webuiRoundedIconsEnabled_
                  ? 'brave-history-embeddings:astrophotography-mode'
                  : 'brave-history-embeddings:heading-old'}"
              >
              </cr-icon>
              ${this.getHeadingText_()}
            </h2>

            ${this.loadingResults_
              ? html`
                  <div class="loading loading-results">
                    <cr-loading-gradient>
                      <svg
                        width="482"
                        height="40"
                      >
                        <clipPath>
                          <rect
                            width="40"
                            height="40"
                            rx="8"
                            ry="8"
                          ></rect>
                          <rect
                            x="55"
                            y="4"
                            width="calc(100% - 55px)"
                            height="14"
                            rx="4"
                            ry="4"
                          ></rect>
                          <rect
                            x="55"
                            y="24"
                            width="calc(78% - 55px)"
                            height="14"
                            rx="4"
                            ry="4"
                          ></rect>
                        </clipPath>
                      </svg>
                    </cr-loading-gradient>
                  </div>
                `
              : html`
                  <div>
                    <div class="result-items">
                      ${this.searchResult_?.items.map(
                        (item, index) => html`
                          <cr-url-list-item
                            url="${item.url}"
                            title="${item.title}"
                            description="${item.urlForDisplay}"
                            as-anchor
                            as-anchor-target="_blank"
                            always-show-suffix
                          >
                            <span
                              class="time"
                              slot="suffix"
                              >${item.shortDateTime}</span
                            >
                            <cr-icon-button
                              slot="suffix"
                              iron-icon="cr:more-vert"
                              data-index="${index}"
                              @click="${this.onMoreActionsClick_}"
                              aria-label="${this.i18n('actionMenuDescription')}"
                              aria-description="${item.title}"
                            >
                            </cr-icon-button>
                          </cr-url-list-item>
                          <hr />
                        `,
                      )}
                    </div>

                    <div class="footer">
                      <div>${this.i18n('braveHistoryEmbeddingsFooter')}</div>
                    </div>
                  </div>
                `}
          </div>
        `
      : ''}

    <cr-lazy-render-lit
      id="sharedMenu"
      .template="${() => html`
        <cr-action-menu
          role-description="${this.i18n('actionMenuDescription')}"
        >
          <button
            id="moreFromSiteOption"
            class="dropdown-item"
            @click="${this.onMoreFromSiteClick_}"
          >
            ${this.i18n('moreFromSite')}
          </button>
          <button
            id="removeFromHistoryOption"
            class="dropdown-item"
            @click="${this.onRemoveFromHistoryClick_}"
          >
            ${this.i18n('removeFromHistory')}
          </button>
        </cr-action-menu>
      `}"
    >
    </cr-lazy-render-lit>
    <!--_html_template_end_-->`
  // clang-format on
}
