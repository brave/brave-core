/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import {
  AccountStateFieldTags,
  whichAccountState,
} from '../brave_account.mojom-webui.js'
import { BraveAccountSettingsStrings } from '../brave_components_webui_strings.js'
import { SettingsBraveAccountRowElement } from './brave_account_row.js'

export function getHtml(this: SettingsBraveAccountRowElement) {
  return html`
    <div class="row-container">
      ${this.state === undefined ? nothing : html`
        ${whichAccountState(this.state) === AccountStateFieldTags.LOGGED_IN
          ? html`
            <div class="first-row">
              <div class="circle">
                <leo-icon name="social-brave-release-favicon-fullheight-color">
                </leo-icon>
              </div>
              <div class="title-and-description">
                <div class="title">
                  ${this.i18n(
                      BraveAccountSettingsStrings.BRAVE_ACCOUNT_TITLE)}
                </div>
                <div class="description">
                  <div id="email">${this.state.loggedIn!.email}</div>
                </div>
              </div>
              <leo-button kind="outline"
                          size="small"
                          @click=${this.onLogOutButtonClicked}>
                <leo-icon name="outside" slot="icon-before"></leo-icon>
                ${this.i18n(
                    BraveAccountSettingsStrings
                         .SETTINGS_BRAVE_ACCOUNT_LOG_OUT_BUTTON_LABEL)}
              </leo-button>
            </div>`
          : this.state.loggedOut!.verification ? html`
            <div class="first-row">
              <div class="circle">
                <leo-icon name="social-brave-release-favicon-fullheight-color">
                </leo-icon>
              </div>
              <div class="title-and-description">
                <div class="title">
                  ${this.i18n(
                      BraveAccountSettingsStrings
                           .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_TITLE)}
                </div>
                <div class="description">
                  <localized-link
                      .localizedString=${`${
                        this.i18n(BraveAccountSettingsStrings
                          .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_1)} ${
                        this.i18n(BraveAccountSettingsStrings
                          .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_2)} ${
                        this.i18nAdvanced(BraveAccountSettingsStrings
                          .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_3,
                          {tags: ['a'], attrs: ['href']})}`}
                      @link-clicked=${this.onResendConfirmationEmailLinkClicked}>
                  </localized-link>
                </div>
              </div>
            </div>
            <div class="second-row">
              <leo-button kind="plain"
                          size="small"
                          @click=${this.openBraveAccountDialog}>
                ${this.i18n(
                    BraveAccountSettingsStrings
                         .SETTINGS_BRAVE_ACCOUNT_ENTER_REGISTRATION_CODE_BUTTON_LABEL)}
              </leo-button>
              <leo-button kind="plain"
                          size="small"
                          class="cancel-registration-button"
                          @click=${this.onCancelRegistrationButtonClicked}>
                ${this.i18n(
                    BraveAccountSettingsStrings
                         .SETTINGS_BRAVE_ACCOUNT_CANCEL_REGISTRATION_BUTTON_LABEL)}
              </leo-button>
            </div>`
          : html`
            <div class="first-row">
              <div class="circle">
                <leo-icon name="social-brave-release-favicon-fullheight-color">
                </leo-icon>
              </div>
              <div class="title-and-description">
                <div class="title">
                  ${this.i18n(
                      BraveAccountSettingsStrings
                           .SETTINGS_BRAVE_ACCOUNT_LOGGED_OUT_ROW_TITLE)}
                </div>
                <div class="description">
                  ${this.i18n(
                      BraveAccountSettingsStrings.BRAVE_ACCOUNT_DESCRIPTION)}
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
      `}
    </div>`
}
