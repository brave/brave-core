/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import { AccountStateFieldTags, whichAccountState } from '../brave_account_row.mojom-webui.js'
import { BraveAccountSettingsStrings } from '../brave_components_webui_strings.js'
import { SettingsBraveAccountRow } from './brave_account_row.js'

const createFirstRow = (
  title: string,
  description: string | string[],
  button?: ReturnType<typeof html>
) => {
  const descriptions = Array.isArray(description)
    ? description
    : [description]
  return html`
    <div class="first-row">
      <div class="circle">
        <leo-icon name="social-brave-release-favicon-fullheight-color">
        </leo-icon>
      </div>
      <div class="title-and-description">
        <div class="title">${title}</div>
        ${descriptions.map(
          desc => html`<div class="description">${desc}</div>`)}
      </div>
      ${button || nothing}
    </div>
  `
}

export function getHtml(this: SettingsBraveAccountRow) {
  const stateHtml: Record<
    AccountStateFieldTags,
    () => ReturnType<typeof html>
  > = {
    [AccountStateFieldTags.LOGGED_IN]: () => createFirstRow(
      this.i18n(
          BraveAccountSettingsStrings
               .SETTINGS_BRAVE_ACCOUNT_LOGGED_IN_ROW_TITLE),
      this.state!.loggedIn!.email,
      html`
        <leo-button kind="outline"
                    size="small"
                    @click=${this.onLogOutButtonClicked}>
          ${this.i18n(
                BraveAccountSettingsStrings
                     .SETTINGS_BRAVE_ACCOUNT_LOG_OUT_BUTTON_LABEL)}
        </leo-button>
      `
    ),
    [AccountStateFieldTags.VERIFICATION]: () => html`
      ${createFirstRow(
        this.i18n(
            BraveAccountSettingsStrings
                .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_TITLE),
        [
          this.i18n(
              BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_1),
          this.i18n(
              BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_2)
        ]
      )}
      <div class="second-row">
        <leo-button kind="outline"
                    size="small"
                    ?isDisabled=${this.isResendingConfirmationEmail}
                    @click=${this.onResendConfirmationEmailButtonClicked}>
          ${this.i18n(
                BraveAccountSettingsStrings
                     .SETTINGS_BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_BUTTON_LABEL)}
        </leo-button>
        <leo-button kind="plain-faint"
                    size="small"
                    class="cancel-registration-button"
                    @click=${this.onCancelRegistrationButtonClicked}>
          ${this.i18n(
                BraveAccountSettingsStrings
                     .SETTINGS_BRAVE_ACCOUNT_CANCEL_REGISTRATION_BUTTON_LABEL)}
        </leo-button>
      </div>
    `,
    [AccountStateFieldTags.LOGGED_OUT]: () => createFirstRow(
      this.i18n(
          BraveAccountSettingsStrings
               .SETTINGS_BRAVE_ACCOUNT_LOGGED_OUT_ROW_TITLE),
      this.i18n(
          BraveAccountSettingsStrings
               .BRAVE_ACCOUNT_DESCRIPTION),
      html`
        <leo-button kind="filled"
                    size="small"
                    @click=${this.onGetStartedButtonClicked}>
          ${this.i18n(
                BraveAccountSettingsStrings
                     .SETTINGS_BRAVE_ACCOUNT_GET_STARTED_BUTTON_LABEL)}
        </leo-button>
      `
    ),
  }

  return html`
    <div class="row-container">
      ${this.state === undefined
        ? nothing
        : stateHtml[whichAccountState(this.state)]()}
    </div>`
}
