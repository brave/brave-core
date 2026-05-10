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
      showErrors
      type="email"
      @input=${this.onInput}
    >
      <div class="label ${this.severity}">
        $i18n{BRAVE_ACCOUNT_EMAIL_INPUT_LABEL}
      </div>
      <div
        class="dropdown ${this.shouldShowDropdown ? 'visible' : ''}"
        slot="errors"
      >
        <!-- Note: .dropdown-content is included in each branch (rather than
             wrapping the entire ternary) to ensure the fadeIn animation triggers
             on content changes. When Lit replaces one branch with another, it
             removes the old .dropdown-content and inserts a new one, causing the
             animation to run.

             freezeWhen directive freezes the previous content while the dropdown
             is collapsing, preventing flashes during animation. -->
        ${freezeWhen(
          !this.shouldShowDropdown,
          this.blockBraveAlias && this.isBraveAlias
            ? html`
                <div class="dropdown-content">
                  <leo-icon name="warning-triangle-filled"></leo-icon>
                  <div>$i18n{BRAVE_ACCOUNT_EMAIL_INPUT_ERROR_MESSAGE}</div>
                </div>
              `
            : html`
                <div class="dropdown-content">
                  <leo-icon name="warning-circle-filled"></leo-icon>
                  <div>
                    ${loadTimeData.getStringF(
                      BraveAccountStrings.BRAVE_ACCOUNT_EMAIL_INPUT_DID_YOU_MEAN,
                      this.suggestion,
                    )}
                  </div>
                </div>
              `,
        )}
      </div>
    </leo-input>
    <!--_html_template_end_-->`
}
