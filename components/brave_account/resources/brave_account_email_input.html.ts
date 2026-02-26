/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountEmailInputElement } from './brave_account_email_input.js'

// Maximum email address length according to this RFC3696 errata:
// https://www.rfc-editor.org/errata/eid1690.
// While maxlength is based on UTF-16 code units (where surrogate pairs count as
// 2 code units), type="email"'s built-in validation filters out non-ASCII
// characters, so maxlength="254" effectively means 254 ASCII characters/254
// bytes. Platform behaviors: desktop and iOS invalidate the value if non-ASCII
// is typed, while Android prevents typing non-ASCII entirely.
const MAX_EMAIL_LENGTH = 254

export function getHtml(this: BraveAccountEmailInputElement) {
  return html`<!--_html_template_start_-->
    <leo-input
      maxlength=${MAX_EMAIL_LENGTH}
      placeholder="$i18n{BRAVE_ACCOUNT_EMAIL_INPUT_PLACEHOLDER}"
      required
      showErrors=${this.blockBraveAlias}
      type="email"
      @input=${this.onInput}
    >
      <div class="label ${this.shouldStyleAsError ? 'error' : ''}">
        $i18n{BRAVE_ACCOUNT_EMAIL_INPUT_LABEL}
      </div>
      <div
        class="dropdown ${this.blockBraveAlias && this.isBraveAlias
          ? 'visible'
          : ''}"
        slot="errors"
      >
        <div
          class="dropdown-content"
          id="brave-alias-dropdown"
        >
          <leo-icon name="warning-triangle-filled"></leo-icon>
          <div>$i18n{BRAVE_ACCOUNT_EMAIL_INPUT_ERROR_MESSAGE}</div>
        </div>
      </div>
    </leo-input>
    <!--_html_template_end_-->`
}
