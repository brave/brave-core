// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { BraveToolbarSearchField } from "./br_toolbar_search_field.js";
import { html } from '//resources/lit/v3_0/lit.rollup.js';

export function getHtml(this: BraveToolbarSearchField) {
  return html`
    <input
      id="pageSearchToggle"
      class="page-search_toggle"
      .checked="${this.showingSearch}"
      @change="${() => this.showingSearch = this.$.pageSearchToggle.checked}"
      type="checkbox" />
    <div class="page-search_content" title="${this.label}">
      <label class="page-search_label" @mousedown="${this.labelMouseDown_}" for="pageSearchToggle">
        <leo-icon name="search"></leo-icon>
      </label>
      <div class="page-search_box">
        <label mousedown="${this.labelMouseDown_}" class="page-search_close-button" for="pageSearchToggle">
          <leo-icon name="close"></leo-icon>
        </label>
        <input id="searchInput"
            class="page-search_text"
            type="search"
            @input="${this.onSearchTermInput}"
            @search="${this.onSearchTermSearch}"
            @keydown="${this.onSearchTermKeydown_}"
            @focus="${this.onInputFocus_}"
            @blur="${this.onInputBlur_}"
            autofocus
            spellcheck="false"
            placeholder="${this.label}"
        />
      </div>
    </div>`
}
