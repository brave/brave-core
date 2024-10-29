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
      ?checked="${this.showingSearch}"
      type="checkbox" />
    <div class="page-search_content" title="${this.label}">
      <label class="page-search_label" @mousedown="${this.labelMouseDown_}" for="pageSearchToggle">
        <svg
          xmlns="http://www.w3.org/2000/svg" width="20" height="20">
          <path fill="#FFF" fill-rule="evenodd" d="M11.771 12.326a.662.662 0 0 1 .165-.148 5.217 5.217 0 1 0-3.566 1.41c1.3-.001 2.488-.476 3.401-1.262zm1.462.39l4.707 4.303a.652.652 0 1 1-.88.962L12.267 13.6a6.522 6.522 0 1 1 .966-.884z"/>
        </svg>
      </label>
      <div class="page-search_box">
        <label mousedown="${this.labelMouseDown_}" class="page-search_close-button" for="pageSearchToggle">
          <svg width="20" height="20"
            xmlns="http://www.w3.org/2000/svg">
            <path d="M10.113 9.23l2.484-2.484a.625.625 0 1 1 .884.883l-2.484 2.484 2.484 2.484a.625.625 0 1 1-.884.884l-2.484-2.484-2.484 2.484a.625.625 0 1 1-.883-.884l2.483-2.484L6.746 7.63a.625.625 0 0 1 .883-.883l2.484 2.483zM10 18.124a8.125 8.125 0 1 1 0-16.25 8.125 8.125 0 0 1 0 16.25zm0-1.25a6.875 6.875 0 1 0 0-13.75 6.875 6.875 0 0 0 0 13.75z" fill="#FFF" fill-rule="evenodd"/>
          </svg>
        </label>
        <input id="searchInput"
            class="page-search_text"
            type="search"
            @input="${this.onSearchTermInput}"
            @search="${(this as any).onSearchTermSearch}"
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
