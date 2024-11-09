/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'
import { SettingsBraveAccountCreateDialogElement } from './brave_account_create_dialog.js'

export function getHtml(this: SettingsBraveAccountCreateDialogElement) {
  return html`<!--_html_template_start_-->
    <settings-brave-account-dialog dialog-description="$i18n{braveSyncBraveAccountDesc}"
                                   dialog-title="Create your account"
                                   show-back-button>
      <div slot="inputs">
        <leo-input placeholder="Enter your email address"
                   showErrors
                   @input=${this.onEmailAddressInput}>
          <div class="label ${this.emailAddress.length !== 0 && !this.isEmailAddressValid
                           || this.isEmailAddressValid && this.emailAddressEndsWithBraveAlias ?
                            'red' : ''}">
            Email address
          </div>
          <div class="dropdown ${this.isEmailAddressValid && this.emailAddressEndsWithBraveAlias ? 'visible' : ''}"
               id="brave-alias-dropdown"
               slot="errors">
            <leo-icon name="warning-triangle-filled"></leo-icon>
            <div>You can't use @bravealias.com addresses for creating Brave accounts. Please try again with a different domain.</div>
          </div>
        </leo-input>
        <leo-input placeholder="Enter a name for your account"
                   @input=${this.onAccountNameInput}>
          <div class="label">Account name</div>
        </leo-input>
        <leo-input placeholder="Enter your password"
                   showErrors
                   type="password"
                   @input=${this.onCreatePasswordInput}>
          <div class="label">Create a password</div>
          <leo-icon name="eye-off"
                    slot="right-icon"
                    @click=${this.OnEyeIconClicked}>
          </leo-icon>
          <div slot="errors" class="dropdown ${this.percent !== 0 ? 'visible' : ''} ${this.getCategory()}"
                             id="password-dropdown">
            <leo-tooltip mode="default"
                         mouseleaveTimeout="0"
                         placement="bottom">
              <div class="password-strength-indicator">
                <div class="password-strength-bar">
                  <div class="password-strength"
                       style="--password-strength: ${this.percent}">
                  </div>
                </div>
                <div class="password-strength-category">
                  ${(() => {
                    switch(this.getCategory()) {
                      case 'weak':
                        return 'Weak'
                      case 'medium':
                        return 'Medium'
                      case 'strong':
                        return 'Strong'
                    }
                  })()}
                </div>
              </div>
              <div slot="content">
                Passwords should have:
                ${this.regexps.map(([_, requirement_met, text]) => html`
                  <div class="password-requirement ${requirement_met ? 'requirement-met' : ''}">
                    <leo-icon name=${requirement_met ? 'check-circle-outline' : 'close-circle'}></leo-icon>
                    ${text}
                  </div>`
                )}
              </div>
            </leo-tooltip>
          </div>
        </leo-input>
        <leo-input placeholder="Confirm your password"
                   showErrors
                   type="password"
                   @input=${this.onConfirmPasswordInput}>
          <div class="label ${this.passwordConfirmation.length !== 0
                           && this.passwordConfirmation !== this.password ?
                              'red' : ''}">
            Confirm password
          </div>
          <leo-icon name="eye-off"
                    slot="right-icon"
                    @click=${this.OnEyeIconClicked}>
          </leo-icon>
          <div class="dropdown ${this.passwordConfirmation.length !== 0 ? 'visible' : ''}"
               id="password-confirmation-dropdown"
               slot="errors">
            <leo-icon name=${this.getIconName()}></leo-icon>
            <div>${`Passwords ${this.icon === 'check-circle-filled' ? '' : 'don\'t'} match`}</div>
          </div>
        </leo-input>
        <leo-checkbox @change=${this.onChange}>
          <div>
            I have read and accept the <a href="#">Terms of service</a> and <a href="#">Privacy agreement</a>.
          </div>
        </leo-checkbox>
      </div>
      <div slot="buttons">
        <leo-button ?isDisabled=${!this.isEmailAddressValid
                               || this.isEmailAddressValid && this.emailAddressEndsWithBraveAlias
                               || !this.isAccountNameValid
                               || this.percent !== 100
                               || this.passwordConfirmation !== this.password
                               || !this.isChecked}>
          Create account
        </leo-button>
      </div>
    </settings-brave-account-dialog>
  <!--_html_template_end_-->`
}
