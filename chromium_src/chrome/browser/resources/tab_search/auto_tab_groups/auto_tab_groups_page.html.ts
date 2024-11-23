// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from '//resources/lit/v3_0/lit.rollup.js';
import type { AutoTabGroupsPageElement } from './auto_tab_groups_page.js';

function getUndoFocusTabsHtml(this:  AutoTabGroupsPageElement) {
  return this.undoTopic_ !== '' ? html`
    <div class="undo" id="undo">
    ${this.getWindowCreatedMessage_()}
    <cr-button id="undoButton" class="action-button" @click="${this.onUndoClicked_}">${this.getUndoButtonLabel_()}</cr-button>
    </div>
  ` : '';
}

function getTopicsHtml(this:  AutoTabGroupsPageElement) {
  return this.topics_.map((entry, index) => html`
    <div>
      <cr-button id="${this.getTopicId_(index)}" data-index="${index}" @click="${this.onTopicClicked_}">
        ${entry}
      </cr-button>
    </div>
  `);
}

export function getHtml(this:  AutoTabGroupsPageElement) {
  return html`<!--_html_template_start_-->
    <div id="brave-tab-focus" class="brave-tab-focus">
      <div id="header"
          class="auto-tab-groups-header"
          aria-live="polite"
          aria-relevant="all">
        ${
          this.showBackButton ? html`
            <cr-icon-button class="back-button"
              aria-label="${'Organize tabs'}"
              iron-icon="cr:arrow-back"
              @click="${this.onBackClick_}">
          </cr-icon-button>
        ` :
                            ''}
        ${this.getTitle_()}
    </div>
        <div class="subtitle">${this.getSubtitle_()}</div>
        <cr-input id="topic-input" class="topic-input" placeholder="${this.getTopicInputPlaceholder_()}" type="text"
                  .value="${this.topic}" @value-changed="${this.onTopicInputChanged_}">
        </cr-input>
        <cr-button id="submitButton" class="action-button" @click="${this.onFocusTabsClicked_}">${this.getSubmitButtonLabel_()}</cr-button>
        ${getUndoFocusTabsHtml.bind(this)()}
        <div class="subtitle">${this.getSuggestedTopicsSubtitle_()}</div>
        ${getTopicsHtml.bind(this)()}
    </div>
    <!--_html_template_end_-->`;
}
