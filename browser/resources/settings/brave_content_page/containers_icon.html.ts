// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { SettingsBraveContentContainersIconElement } from './containers_icon.js'

export function getHtml(this: SettingsBraveContentContainersIconElement) {
  return html`
    <div
      class="icon-container ${this.selected ? 'selected' : ''}"
      style="background-color: ${this.backgroundColor}"
      @click="${this.handleIconClick_}"
    >
      <leo-icon name="${this.leoIcon}"></leo-icon>
    </div>
  `
}
