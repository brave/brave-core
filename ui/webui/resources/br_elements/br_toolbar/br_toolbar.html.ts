// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { CrToolbarElement } from "./br_toolbar.js";
import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js';

export function getHtml(this: CrToolbarElement) {
  return html`
<link rel="stylesheet" href="chrome://resources/brave/fonts/poppins.css">
<link rel="stylesheet" href="chrome://resources/brave/fonts/inter.css">

<div class="br-toolbar ${this.fontsLoadedClassName}">
  ${this.showMenu ? html`
    <div id="menuButtonContainer">
      <cr-icon-button id="menuButton"
          iron-icon="cr20:menu" @click="${this.onMenuClick_}"
          aria-label="${this.menuLabel || nothing}"
          title="${this.menuLabel}">
      </cr-icon-button>
    </div>` : ''}
  <ul class="nav-items" role="navigation" aria-label="Category navigation">
    <li class="nav-items-list-item" title="${this.settingsTitle}">
      <a class="nav-item ${this.getNavItemSelectedClassName('settings')}" href="chrome://settings">
        <span class="nav-item-icon">
          <leo-icon name="settings"></leo-icon>
        </span>
        <span class="nav-item-text">${this.settingsTitle}</span>
      </a>
    </li>
      ${this.shouldShowRewardsButton_ ? html`<li class="nav-items-list-item" title="${this.braveRewardsTitle}">
        <a class="nav-item ${this.getNavItemSelectedClassName('rewards')}" href="chrome://rewards">
          <span class="nav-item-icon">
            <leo-icon name="product-bat-outline"></leo-icon>
          </span>
          <span class="nav-item-text">${this.braveRewardsTitle}</span>
        </a>
      </li>` : ''}
    <li class="nav-items-list-item" title="${this.historyTitle}">
      <a class="nav-item ${this.getNavItemSelectedClassName('history')}" href="chrome://history">
        <span class="nav-item-icon">
            <leo-icon name="history"></leo-icon>
        </span>
        <span class="nav-item-text">${this.historyTitle}</span>
      </a>
    </li>
    <li class="nav-items-list-item" title="${this.bookmarksTitle}">
      <a class="nav-item ${this.getNavItemSelectedClassName('bookmarks')}" href="chrome://bookmarks">
        <span class="nav-item-icon">
          <leo-icon name="product-bookmarks"></leo-icon>
        </span>
        <span class="nav-item-text">${this.bookmarksTitle}</span>
      </a>
    </li>
    <li class="nav-items-list-item" title="${this.downloadsTitle}">
      <a class="nav-item ${this.getNavItemSelectedClassName('downloads')}" href="chrome://downloads">
        <span class="nav-item-icon">
          <leo-icon name="download"></leo-icon>
        </span>
        <span class="nav-item-text">${this.downloadsTitle}</span>
      </a>
    </li>
    ${this.isBraveWalletAllowed_ ? html`<li class="nav-items-list-item" title="${this.walletsTitle}">
      <a class="nav-item ${this.getNavItemSelectedClassName('wallet')}" href="chrome://wallet">
        <span class="nav-item-icon">
          <leo-icon name="product-brave-wallet"></leo-icon>
        </span>
        <span class="nav-item-text">${this.walletsTitle}</span>
      </a>
    </li>` : ''}
  </ul>
    <br-toolbar-search-field id="search"
      ?narrow="${this.narrow}"
      label="${this.searchPrompt}" clear-label="${this.clearLabel}"
      ?spinner-active="${this.spinnerActive}"
      ?showing-search="${this.showingSearch}"
    ></br-toolbar-search-field>
  <div class="toolbar-extra">
    <slot></slot>
  </div>
</div>
`
}
