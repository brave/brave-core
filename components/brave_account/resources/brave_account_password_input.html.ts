/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_password_icons.js'
import './brave_account_password_strength_meter.js'
import {
  BraveAccountPasswordInputElement,
  MAX_PASSWORD_LENGTH,
} from './brave_account_password_input.js'

export function getHtml(this: BraveAccountPasswordInputElement) {
  return html`<!--_html_template_start_-->
    <leo-input
      maxlength=${MAX_PASSWORD_LENGTH}
      placeholder=${this.placeholder}
      ?showErrors=${this.config.mode !== 'regular'}
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
        <div class="dropdown-content">
          ${(() => {
            switch (this.config.mode) {
              case 'confirmation':
                return html`
                  <div id="password-confirmation-dropdown">
                    <leo-icon name=${this.getIconName()}></leo-icon>
                    <div>
                      ${this.icon === 'check-circle-filled'
                        ? html`$i18n{BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_SUCCESS_MESSAGE}`
                        : html`$i18n{BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_ERROR_MESSAGE}`}
                    </div>
                  </div>
                `
              case 'strength':
                return html`<brave-account-password-strength-meter
                  password=${this.password}
                  @password-strength-changed=${this.onPasswordStrengthChanged}
                >
                </brave-account-password-strength-meter>`
              default:
                return nothing
            }
          })()}
        </div>
      </div>
    </leo-input>
    <!--_html_template_end_-->`
}
