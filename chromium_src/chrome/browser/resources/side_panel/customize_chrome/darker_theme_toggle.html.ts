// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from '//resources/lit/v3_0/lit.rollup.js'
import type { DarkerThemeToggleElement } from './app.js'

export function getHtml(this: DarkerThemeToggleElement) {
  return html`
    <div id="darker-theme-toggle-container">
      <leo-icon name="theme-darker"></leo-icon>
      <span>${this.i18n('CUSTOMIZE_CHROME_DARKER_THEME_TOGGLE_LABEL')}</span>
      <!-- Use cr-toggle instead of leo-toggle in order to inherit style -->
      <cr-toggle
        .checked="${this.usingDarkerTheme_}"
        @change="${this.onDarkerThemeToggleChange}"
      ></cr-toggle>
    </div>
  `
}
