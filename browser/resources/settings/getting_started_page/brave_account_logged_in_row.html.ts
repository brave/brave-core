/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '//resources/cr_components/localized_link/localized_link.js'
import { assert } from '//resources/js/assert.js'
import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountLoggedInRowElement } from './brave_account_logged_in_row.js'
import { BraveAccountSettingsStrings } from '../brave_components_webui_strings.js'
import { LoggedInVerificationIntent } from '../brave_account.mojom-webui.js'

export function getHtml(this: BraveAccountLoggedInRowElement) {
  // Logged-in verification is always a password change - there is no other
  // `LoggedInVerificationIntent` - so the rest of this template branches only
  // on `verifiedEmail`.
  assert(
    !this.state.verification
    || this.state.verification.intent
        === LoggedInVerificationIntent.kChangePassword)

  return this.state.verification
    ? html`
      <div class="first-row">
        <leo-icon name="social-brave-release-favicon-fullheight-color">
        </leo-icon>
        <div class="title-and-description">
          <div class="title">
            ${this.i18n(
                BraveAccountSettingsStrings
                     .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_TITLE)}
          </div>
          <div class="description">
            ${this.state.verification.verifiedEmail
              ? this.i18n(BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_CHANGE_PASSWORD_VERIFIED_ROW_DESCRIPTION)
              : html`
                <localized-link
                    .localizedString=${`${
                      this.i18n(BraveAccountSettingsStrings
                        .SETTINGS_BRAVE_ACCOUNT_CHANGE_PASSWORD_ROW_DESCRIPTION_1)} ${
                      this.i18n(
                        BraveAccountSettingsStrings
                             .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_2)} ${
                      this.i18nAdvanced(BraveAccountSettingsStrings
                        .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_3,
                        {tags: ['a'], attrs: ['href']})}`}
                    @link-clicked=${this.onResendConfirmationEmailLinkClicked}>
                </localized-link>`}
          </div>
        </div>
      </div>
      <div class="second-row">
        <leo-button kind="plain"
                    size="small"
                    @click=${this.openBraveAccountDialog}>
          ${this.i18n(this.state.verification.verifiedEmail
              ? BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_SET_NEW_PASSWORD_BUTTON_LABEL
              : BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_ENTER_VERIFICATION_CODE_BUTTON_LABEL)}
        </leo-button>
        <leo-button kind="plain"
                    size="small"
                    class="cancel-verification-button"
                    @click=${this.onCancelVerificationButtonClicked}>
          ${this.i18n(
              BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_CANCEL_CHANGE_PASSWORD_BUTTON_LABEL)}
        </leo-button>
      </div>`
    : html`
      <div class="first-row">
        <leo-icon name="social-brave-release-favicon-fullheight-color">
        </leo-icon>
        <div class="title-and-description">
          <div class="title">
            ${this.i18n(BraveAccountSettingsStrings.BRAVE_ACCOUNT_TITLE)}
          </div>
          <div class="description">
            <div id="email">${this.state.email}</div>
          </div>
        </div>
        <leo-button kind="plain"
                    size="small"
                    ?isDisabled=${this.isChangingPassword}
                    @click=${this.onChangePasswordButtonClicked}>
          ${this.i18n(
              BraveAccountSettingsStrings
                   .SETTINGS_BRAVE_ACCOUNT_CHANGE_PASSWORD_BUTTON_LABEL)}
        </leo-button>
        <leo-button kind="outline"
                    size="small"
                    @click=${this.onLogOutButtonClicked}>
          <leo-icon name="outside" slot="icon-before"></leo-icon>
          ${this.i18n(
              BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_LOG_OUT_BUTTON_LABEL)}
        </leo-button>
      </div>`
}
