/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { BraveAccountLoggedOutRowElement } from './brave_account_logged_out_row.js'
import { ROW_BUTTON_SIZE } from './brave_account_row_base.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'
import { LoggedOutVerificationIntent } from './brave_account.mojom-webui.js'

export function getHtml(this: BraveAccountLoggedOutRowElement) {
  return html`${this.state.verification
    ? html` <div class="first-row">
          <if expr="not is_android and not is_ios">
            <leo-icon name="social-brave-release-favicon-fullheight-color">
            </leo-icon>
          </if>
          <div class="title-and-description">
            <div class="title">
              ${loadTimeData.getString(
                BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_TITLE,
              )}
            </div>
            <div class="description">
              ${
                this.state.verification.intent
                  === LoggedOutVerificationIntent.kResetPassword
                && this.state.verification.verifiedEmail
                  ? html`${loadTimeData.getString(
                      BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_RESET_PASSWORD_VERIFIED_ROW_DESCRIPTION,
                    )}`
                  : html`${this.getVerificationDescription().beforeLink}
                    <if expr="not is_android and not is_ios">
                      <leo-link
                        @click=${this.onResendConfirmationEmailLinkClicked}
                      >
                    </if>
                    ${this.getVerificationDescription().linkLabel}
                    <if expr="not is_android and not is_ios">
                      </leo-link>
                    </if>
                    ${this.getVerificationDescription().afterLink}`
              }
            </div>
          </div>
        </div>
        <div class="second-row">
          <leo-button
<if expr="not is_android and not is_ios">
            kind="plain"
</if>
<if expr="is_android or is_ios">
            kind="filled"
</if>
            size=${ROW_BUTTON_SIZE}
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
<if expr="is_android or is_ios">
          ${
            this.state.verification.intent
              !== LoggedOutVerificationIntent.kResetPassword
            || !this.state.verification.verifiedEmail
              ? html`<leo-button
                  kind="plain"
                  size=${ROW_BUTTON_SIZE}
                  ?isDisabled=${this.isResendingConfirmationEmail}
                  @click=${this.onResendConfirmationEmailLinkClicked}
                >
                  ${loadTimeData.getString(
                    BraveAccountSettingsStrings.BRAVE_ACCOUNT_RESEND_EMAIL_CODE_BUTTON_LABEL,
                  )}
                </leo-button>`
              : nothing
          }
</if>
          <leo-button
            kind="plain"
            size=${ROW_BUTTON_SIZE}
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
        <if expr="not is_android and not is_ios">
          <leo-icon name="social-brave-release-favicon-fullheight-color">
          </leo-icon>
        </if>
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
          size=${ROW_BUTTON_SIZE}
          @click=${this.openDialogInDefaultMode}
        >
          ${loadTimeData.getString(
            BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_GET_STARTED_BUTTON_LABEL,
          )}
        </leo-button>
      </div>`}`
}
