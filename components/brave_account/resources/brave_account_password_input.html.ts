/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_password_icons.js'
import './brave_account_password_strength_meter.js'
import {
  BraveAccountPasswordInputElement,
  freezeWhen,
  MAX_PASSWORD_LENGTH,
} from './brave_account_password_input.js'

export function getHtml(this: BraveAccountPasswordInputElement) {
  return html`<!--_html_template_start_-->
    <leo-input
      maxlength=${MAX_PASSWORD_LENGTH}
      placeholder=${this.placeholder}
      showErrors
      type="password"
      @blur=${this.focusHandler}
      @focus=${this.focusHandler}
      @input=${this.onInput}
      @toggle-visibility=${this.onToggleVisibility}
    >
      <div class="label-container">
        <div class="label ${this.shouldStyleAsError ? 'error' : ''}">
          ${this.label}
        </div>
        <slot name="label-extra"></slot>
      </div>
      <brave-account-password-icons
        slot="right-icon"
        .isCapsLockOn=${this.isCapsLockOn}
        .isInputFocused=${this.isInputFocused}
      >
      </brave-account-password-icons>
      <div
        class="dropdown ${this.shouldShowDropdown ? 'visible' : ''}"
        slot="errors"
      >
        <!-- Note: .dropdown-content is included in each branch (rather than
             wrapping the entire ternary) to ensure the fadeIn animation triggers
             on content changes. When Lit replaces one branch with another, it
             removes the old .dropdown-content and inserts a new one, causing the
             animation to run.

             freezeWhen directive freezes the previous content when password
             is empty (dropdown collapsing), preventing flashes during animation. -->
        ${freezeWhen(
          this.password.length === 0,
          this.config.mode === 'confirmation'
            ? this.password === this.confirmPassword
              ? html`
                  <div class="dropdown-content">
                    <leo-icon name="check-circle-filled"></leo-icon>
                    <div>
                      $i18n{BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_SUCCESS_MESSAGE}
                    </div>
                  </div>
                `
              : html`
                  <div class="dropdown-content">
                    <leo-icon name="warning-triangle-filled"></leo-icon>
                    <div>
                      $i18n{BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_ERROR_MESSAGE}
                    </div>
                  </div>
                `
            : this.config.mode === 'strength' && this.isValid
              ? html`<div class="dropdown-content">
                  <brave-account-password-strength-meter
                    password=${this.password}
                    @password-strength-changed=${this.onPasswordStrengthChanged}
                  >
                  </brave-account-password-strength-meter>
                </div>`
              : html`
                  <div class="dropdown-content">
                    <leo-icon name="warning-triangle-filled"></leo-icon>
                    <div>
                      $i18n{BRAVE_ACCOUNT_PASSWORD_INPUT_WHITESPACE_ERROR_MESSAGE}
                    </div>
                  </div>
                `,
        )}
      </div>
    </leo-input>
    <!--_html_template_end_-->`
}
