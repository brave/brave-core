// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from '//resources/lit/v3_0/lit.rollup.js'
import type { TabFocusPageElement } from './tab_focus_page.js'

export function getHtml(this: TabFocusPageElement) {
  return this.showFRE_
    ? html`
        <div class="enable-tab-focus-wrapper">
          <div class="title-row">
            <div class="title-column">
              <div class="title">${this.getTitle_()}</div>
              ${!this.showFRE_
                ? html` <div class="subtitle">${this.getSubtitle_()}</div> `
                : ''}
            </div>
          </div>
          <div class="enable-tab-focus-content-wrapper">
            <div class="enable-tab-focus-illustration"></div>
            <div class="enable-tab-focus-info-wrapper">
              <span class="enable-tab-focus-info-text">
                ${this.getPrivacyDisclaimerMessage_()}
              </span>
              <div class="enable-tab-focus-button-row">
                <span
                  class="learn-more-link"
                  @click=${this.onLearnMoreClick_}
                >
                  ${this.getLearnMoreLabel_()}
                </span>
                <div>
                  <leo-button
                    id="enableButton"
                    data-testid="enable-tab-focus"
                    kind="filled"
                    size="small"
                    @click="${this.onEnableTabFocusClick_}"
                  >
                    ${this.getEnableButtonLabel_()}
                  </leo-button>
                </div>
              </div>
            </div>
          </div>
        </div>
      `
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
            <div class="title-row">
              <div class="title-column">
                <div class="title">${this.getTitle_()}</div>
                ${!this.showFRE_
                  ? html` <div class="subtitle">${this.getSubtitle_()}</div> `
                  : ''}
              </div>
            </div>
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
            ${this.undoTopic_ !== ''
              ? html`
                  <leo-alert
                    id="undo"
                    type="success"
                    hideIcon="{true}"
                  >
                    ${this.getWindowCreatedMessage_()}
                    <leo-button
                      id="undoButton"
                      kind="plain-faint"
                      size="tiny"
                      @click="${this.onUndoClick_}"
                    >
                      ${this.getUndoButtonLabel_()}
                    </leo-button>
                  </leo-alert>
                `
              : ''}
          </div>
          ${this.errorMessage === ''
            ? html`
                <div class="topics-wrapper">
                  <div class="topics-title">
                    ${this.getSuggestedTopicsSubtitle_()}
                  </div>
                  ${this.isLoadingTopics
                    ? [1, 2, 3, 4, 5].map(
                        (key) => html`
                          <leo-button
                            key=${key}
                            class="topics-button"
                            size="small"
                            kind="outline"
                            isDisabled="{true}"
                          >
                            <div class="topic-description">
                              <div class="emoji-wrapper">
                                <leo-progressring
                                  class="loading-ring"
                                ></leo-progressring>
                              </div>
                              <div class="empty-state"></div>
                            </div>
                          </leo-button>
                        `,
                      )
                    : this.topics_.map(
                        (entry, index) => html`
                          <leo-button
                            id="${this.getTopicId_(index)}"
                            data-testid="${this.getTopicId_(index)}"
                            data-index="${index}"
                            class="topics-button"
                            size="small"
                            kind="outline"
                            @click=${this.onTopicClick_}
                          >
                            <div class="topic-description">
                              <div class="emoji-wrapper">
                                ${this.getTopicEmoji_(entry)}
                              </div>
                              ${this.getTopicWithoutEmoji_(entry)}
                            </div>
                          </leo-button>
                        `,
                      )}
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
