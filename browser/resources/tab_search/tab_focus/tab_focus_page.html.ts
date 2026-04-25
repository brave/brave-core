// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from '//resources/lit/v3_0/lit.rollup.js'
import type { TabFocusPageElement } from './tab_focus_page.js'

export function getHtml(this: TabFocusPageElement) {
  return this.showFRE_
    ? this.getEnableTabFocusHtml_()
    : html`<!--_html_template_start_-->
        <div
          id="brave-tab-focus"
          class="brave-tab-focus"
        >
          <div
            id="header"
            class="auto-tab-groups-header"
            aria-live="polite"
            aria-relevant="all"
          >
            ${this.getHeaderHtml_()}
            <div class="input-row">
              <leo-input
                id="topic-input"
                class="topic-input"
                placeholder="${this.getTopicInputPlaceholder_()}"
                type="text"
                value=${this.topic}
                @input=${this.onTopicInputChanged_}
                ?disabled=${this.errorMessage !== ''}
              >
              </leo-input>
              <leo-button
                id="submitButton"
                size="medium"
                fab
                @click=${this.onFocusTabsClick_}
                ?isDisabled=${this.errorMessage !== ''}
              >
                ${this.getSubmitButtonLabel_()}
              </leo-button>
            </div>
            ${this.getUndoFocusTabsHtml_()}
          </div>
          ${this.errorMessage === ''
            ? html`
                <div class="topics-wrapper">
                  <div class="topics-title">
                    ${this.getSuggestedTopicsSubtitle_()}
                  </div>
                  ${this.getTopicsHtml_()}
                </div>
              `
            : html`
                <div class="error-wrapper">
                  <leo-alert
                    id="error"
                    type="error"
                  >
                    ${this.errorMessage}
                    ${this.needsPremium
                      ? html`
                          <div slot="actions">
                            <leo-button
                              id="premiumButton"
                              kind="filled"
                              size="medium"
                              @click="${this.onGoPremiumClick_}"
                            >
                              ${this.getGoPremiumButtonLabel_()}
                            </leo-button>
                            <leo-button
                              id="dismissButton"
                              kind="plain-faint"
                              size="medium"
                              @click="${this.onDismissErrorClick_}"
                            >
                              ${this.getDismissButtonLabel_()}
                            </leo-button>
                          </div>
                        `
                      : ''}
                  </leo-alert>
                </div>
              `}
          ${this.errorMessage === ''
            ? html`
                <div class="footer">
                  ${this.getFooterMessage_()}
                  <span
                    class="learn-more-link"
                    @click=${this.onLearnMoreClick_}
                  >
                    ${this.getLearnMoreLabel_()}
                  </span>
                </div>
              `
            : ''}
        </div>
        <!--_html_template_end_-->`
}
