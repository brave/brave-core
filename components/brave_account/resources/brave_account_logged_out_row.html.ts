/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { BraveAccountLoggedOutRowElement } from './brave_account_logged_out_row.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'
import { LoggedOutVerificationIntent } from './brave_account.mojom-webui.js'

export function getHtml(this: BraveAccountLoggedOutRowElement) {
  return html`${this.state.verification
    ? html` <div class="first-row">
          <leo-icon name="social-brave-release-favicon-fullheight-color">
          </leo-icon>
          <div class="title-and-description">
            <div class="title">
              ${loadTimeData.getString(
                BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_TITLE,
              )}
            </div>
            <div class="description">
              ${this.state.verification.intent
                === LoggedOutVerificationIntent.kResetPassword
              && this.state.verification.verifiedEmail
                ? loadTimeData.getString(
                    BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_RESET_PASSWORD_VERIFIED_ROW_DESCRIPTION,
                  )
                : html`${this.getVerificationDescription().beforeLink}<leo-link
                      @click=${this.onResendConfirmationEmailLinkClicked}
                      >${this.getVerificationDescription().linkLabel}</leo-link
                    >${this.getVerificationDescription().afterLink}`}
            </div>
          </div>
        </div>
        <div class="second-row">
          <leo-button
            kind="plain"
            size="small"
            @click=${this.openDialogInDefaultMode}
          >
            ${loadTimeData.getString(
              this.state.verification.intent
                === LoggedOutVerificationIntent.kResetPassword
                && this.state.verification.verifiedEmail
                ? BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_SET_NEW_PASSWORD_BUTTON_LABEL
                : BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_ENTER_VERIFICATION_CODE_BUTTON_LABEL,
            )}
          </leo-button>
          <leo-button
            kind="plain"
            size="small"
            class="cancel-verification-button"
            @click=${this.onCancelVerificationButtonClicked}
          >
            ${loadTimeData.getString(
              this.state.verification.intent
                === LoggedOutVerificationIntent.kResetPassword
                ? BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_CANCEL_RESET_PASSWORD_BUTTON_LABEL
                : BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_CANCEL_REGISTRATION_BUTTON_LABEL,
            )}
          </leo-button>
        </div>`
    : html` <div class="first-row">
        <leo-icon name="social-brave-release-favicon-fullheight-color">
        </leo-icon>
        <div class="title-and-description">
          <div class="title">
            ${loadTimeData.getString(
              BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_LOGGED_OUT_ROW_TITLE,
            )}
          </div>
          <div class="description">
            <if expr="not is_android and not is_ios">
              ${this.getLoggedOutDescription().beforeLink}<leo-link
                href=${loadTimeData.getString('braveAccountLearnMoreURL')}
                target="_blank"
                >${this.getLoggedOutDescription().linkLabel}</leo-link
              >${this.getLoggedOutDescription().afterLink}
            </if>
            <if expr="is_android or is_ios">
              ${loadTimeData.getString(
                BraveAccountSettingsStrings.BRAVE_ACCOUNT_DESCRIPTION,
              )}
            </if>
          </div>
        </div>
        <leo-button
          kind="filled"
          size="small"
          @click=${this.openDialogInDefaultMode}
        >
          ${loadTimeData.getString(
            BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_GET_STARTED_BUTTON_LABEL,
          )}
        </leo-button>
      </div>`}`
}
