// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import {SettingsBraveContentContainersBackgroundChipElement} from './containers_background_chip.js'

export function getHtml(
  this: SettingsBraveContentContainersBackgroundChipElement,
) {
  return html`
    <div
      class="chip-container ${this.selected ? 'selected' : ''}"
      @click="${this.handleChipClick_}"
    >
      <div
        class="chip-content"
        style="background-color: ${this.backgroundColor}"
      ></div>
    </div>
  `
}
