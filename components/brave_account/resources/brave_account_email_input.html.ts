/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { freezeWhen } from './brave_account_common.js'
import { BraveAccountStrings } from './brave_components_webui_strings.js'
import {
  BraveAccountEmailInputElement,
  MAX_EMAIL_LENGTH,
} from './brave_account_email_input.js'

export function getHtml(this: BraveAccountEmailInputElement) {
  return html`<!--_html_template_start_-->
    <leo-input
      maxlength=${MAX_EMAIL_LENGTH}
      placeholder="$i18n{BRAVE_ACCOUNT_EMAIL_INPUT_PLACEHOLDER}"
      ?showErrors=${this.blockBraveAlias || this.suggestion !== ''}
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
        <div class="dropdown-content">
          <leo-icon name="warning-triangle-filled"></leo-icon>
          <div>$i18n{BRAVE_ACCOUNT_EMAIL_INPUT_ERROR_MESSAGE}</div>
        </div>
      </div>
      <div
        class="dropdown ${this.suggestion !== '' ? 'visible' : ''}"
        slot="errors"
      >
        <div class="dropdown-content">
          <leo-icon name="warning-triangle-filled"></leo-icon>
          <div>
            ${freezeWhen(
              !this.suggestion,
              loadTimeData.getStringF(
                BraveAccountStrings.BRAVE_ACCOUNT_EMAIL_INPUT_DID_YOU_MEAN,
                this.suggestion,
              ),
            )}
          </div>
        </div>
      </div>
    </leo-input>
    <!--_html_template_end_-->`
}
