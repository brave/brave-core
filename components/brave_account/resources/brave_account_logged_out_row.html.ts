/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// TODO: localized_link asserts !is_ios, so it cannot be imported here now that
// these rows are also served as a page on mobile. The <localized-link> below is
// left unresolved on every platform - it renders as inert, without its link.
// Needs a replacement that works everywhere.
//
// The logged-out description lost its "Learn more" link outright: the URL came
// from braveAccountLearnMoreURL, which only the brave://settings data source
// adds, so reading it threw here and left the row blank. The mobile copy of the
// description carries no link anyway.
import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountLoggedOutRowElement } from './brave_account_logged_out_row.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'
import { LoggedOutVerificationIntent } from './brave_account.mojom-webui.js'

export function getHtml(this: BraveAccountLoggedOutRowElement) {
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
            ${this.state.verification.intent
                === LoggedOutVerificationIntent.kResetPassword
                && this.state.verification.verifiedEmail
              ? this.i18n(BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_RESET_PASSWORD_VERIFIED_ROW_DESCRIPTION)
              : html`
                <localized-link
                    .localizedString=${`${
                      this.i18n(this.state.verification.intent
                        === LoggedOutVerificationIntent.kResetPassword
                        ? BraveAccountSettingsStrings
                            .SETTINGS_BRAVE_ACCOUNT_RESET_PASSWORD_ROW_DESCRIPTION_1
                        : BraveAccountSettingsStrings
                            .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_1)} ${
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
          ${this.i18n(this.state.verification.intent
              === LoggedOutVerificationIntent.kResetPassword
              && this.state.verification.verifiedEmail
              ? BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_SET_NEW_PASSWORD_BUTTON_LABEL
              : BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_ENTER_VERIFICATION_CODE_BUTTON_LABEL)}
        </leo-button>
        <leo-button kind="plain"
                    size="small"
                    class="cancel-verification-button"
                    @click=${this.onCancelVerificationButtonClicked}>
          ${this.i18n(this.state.verification.intent
              === LoggedOutVerificationIntent.kResetPassword
              ? BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_CANCEL_RESET_PASSWORD_BUTTON_LABEL
              : BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_CANCEL_REGISTRATION_BUTTON_LABEL)}
        </leo-button>
      </div>`
    : html`
      <div class="first-row">
        <leo-icon name="social-brave-release-favicon-fullheight-color">
        </leo-icon>
        <div class="title-and-description">
          <div class="title">
            ${this.i18n(
                BraveAccountSettingsStrings
                     .SETTINGS_BRAVE_ACCOUNT_LOGGED_OUT_ROW_TITLE)}
          </div>
          <div class="description">
            ${this.i18n(
                BraveAccountSettingsStrings
                     .SETTINGS_BRAVE_ACCOUNT_LOGGED_OUT_ROW_DESCRIPTION)}
          </div>
        </div>
        <leo-button kind="filled"
                    size="small"
                    @click=${this.openBraveAccountDialog}>
          ${this.i18n(
              BraveAccountSettingsStrings
                   .SETTINGS_BRAVE_ACCOUNT_GET_STARTED_BUTTON_LABEL)}
        </leo-button>
      </div>`
}
