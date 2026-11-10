/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountLinkRowElement } from './brave_account_link_row.js'

export function getHtml(this: BraveAccountLinkRowElement) {
  return html`<div
    class="row"
    @click=${this.openSettingsPage}
  >
    <leo-icon name=${this.icon}></leo-icon>
    <div class="title-and-description">
      <div class="title">${this.label}</div>
      ${this.description
        ? html`<div class="description">${this.displayDescription}</div>`
        : nothing}
    </div>
    <leo-button
      kind="plain-faint"
      size="medium"
      fab
    >
      <leo-icon name="carat-right"></leo-icon>
    </leo-button>
  </div>`
}
