/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { SettingsBraveAccountRowElement } from './brave_account_row.js'

export function getHtml(this: SettingsBraveAccountRowElement) {
  return html`
    <div class="row-container">
      ${this.getStateHtml()}
    </div>`
}
