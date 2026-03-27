/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountOtpInputElement } from './brave_account_otp_input.js'

// Note: disable iOS text assistance (spellcheck="false").
// It may perform replacement edits (for example smart punctuation),
// inserting invalid characters and replacing valid ones, corrupting the OTP.
// Example sequence:
//   beforeinput: insertText            "."
//   beforeinput: insertText            "."
//   beforeinput: deleteContentBackward
//   beforeinput: insertText            "…"
export function getHtml(this: BraveAccountOtpInputElement) {
  return html`<!--_html_template_start_-->
    <div class="label">$i18n{BRAVE_ACCOUNT_OTP_INPUT_LABEL}</div>
    <div
      class="otp-inputs"
      @paste=${this.onPaste}
    >
      ${this.indices.map(
        (index) => html`
          <leo-input
            autofocus=${index === 0}
<if expr="is_ios">
            spellcheck="false"
</if>
            type="text"
            @beforeinput=${(e: InputEvent) => this.onBeforeInput(e, index)}
            @focus=${this.onFocus}
            @input=${(detail: { value: string }) => this.onInput(detail, index)}
            @keydown=${(detail: { innerEvent: KeyboardEvent }) =>
              this.onKeyDown(detail, index)}
          >
          </leo-input>
        `,
      )}
    </div>
    <!--_html_template_end_-->`
}
