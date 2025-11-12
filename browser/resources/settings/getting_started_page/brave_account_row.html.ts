/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import { AccountState } from '../brave_account_row.mojom-webui.js'
import { loadTimeData } from '//resources/js/load_time_data.js'
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
    AccountState,
    () => ReturnType<typeof html>
  > = {
    [AccountState.kLoggedIn]: () => createFirstRow(
      'John Doe',
      'johndoe@gmail.com',
      html`
        <leo-button kind="outline"
                    size="small"
                    @click=${this.onLogOutButtonClicked}>
          ${loadTimeData.getString('braveAccountLogOutButtonLabel')}
        </leo-button>
      `
    ),
    [AccountState.kVerification]: () => html`
      ${createFirstRow(
        loadTimeData.getString('braveAccountVerificationRowTitle'),
        [
          loadTimeData.getString('braveAccountVerificationRowDescription1'),
          loadTimeData.getString('braveAccountVerificationRowDescription2')
        ]
      )}
      <div class="second-row">
        <leo-button kind="outline"
                    size="small"
                    @click=${this.onResendConfirmationEmailButtonClicked}>
          ${loadTimeData.getString(
            'braveAccountResendConfirmationEmailButtonLabel')}
        </leo-button>
        <leo-button kind="plain-faint"
                    size="small"
                    class="cancel-registration-button"
                    @click=${this.onCancelRegistrationButtonClicked}>
          ${loadTimeData.getString('braveAccountCancelRegistrationButtonLabel')}
        </leo-button>
      </div>
    `,
    [AccountState.kLoggedOut]: () => createFirstRow(
      loadTimeData.getString('braveAccountLoggedOutRowTitle'),
      loadTimeData.getString('braveAccountLoggedOutRowDescription'),
      html`
        <leo-button kind="filled"
                    size="small"
                    @click=${this.onGetStartedButtonClicked}>
          ${loadTimeData.getString('braveAccountGetStartedButtonLabel')}
        </leo-button>
      `
    ),
  }

  return html`
    <div class="row-container">
      ${this.state === undefined ? nothing : stateHtml[this.state]()}
    </div>`
}
