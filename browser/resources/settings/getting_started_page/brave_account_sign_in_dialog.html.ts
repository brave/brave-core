/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js'
import { html } from '//resources/lit/v3_0/lit.rollup.js'
import type { SettingsBraveAccountSignInDialogElement } from './brave_account_sign_in_dialog.js'

export function getHtml(this: SettingsBraveAccountSignInDialogElement) {
  return html`<!--_html_template_start_-->
    <settings-brave-account-dialog show-back-button
      header-text-top="Sign in to your account"
      header-text-bottom="$i18n{braveSyncBraveAccountDesc}">
      <div slot="inputs">
        <leo-input id="email" placeholder="Enter your email address" @input=${this.onEmailInput}>
          <div class="label">Email address</div>
        </leo-input>
        <leo-input id="password" type="password" placeholder="Enter your password" @input=${this.onPasswordInput}>
          <div class="password">
            <div class="label">Password</div>
            <div class="forgot-password" @click=${() => this.fire('forgot-password-button-clicked')}>Forgot your password?</div>
          </div>
          <leo-icon slot="right-icon"
                    id="icon"
                    name="eye-off"
                    @click=${this.show}>
          </leo-icon>
        </leo-input>
      </div>
      <div slot="buttons">
        <leo-button size="medium" ?isDisabled=${!this.isEmailValid || !this.isPasswordValid}>
          Sign in
        </leo-button>
      </div>
    </settings-brave-account-dialog>
  <!--_html_template_end_-->`
}
