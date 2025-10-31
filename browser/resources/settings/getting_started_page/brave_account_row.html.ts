/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import { loadTimeData } from '//resources/js/load_time_data.js'
import { SettingsBraveAccountRow } from './brave_account_row.js'

export function getHtml(this: SettingsBraveAccountRow) {
  return html`
    <div class="row">
      <div class="circle">
        <leo-icon name="social-brave-release-favicon-fullheight-color">
        </leo-icon>
      </div>
      <div class="title-and-description">
        <div class="title">
          ${this.state === 'LOGGED_IN'
            ? 'John Doe'
            : this.state === 'VERIFICATION'
            ? 'Almost there!'
            : loadTimeData.getString('braveAccountRowTitle')}
        </div>
        <div class="description">
          ${this.state === 'LOGGED_IN'
            ? 'johndoe@gmail.com'
            : this.state === 'VERIFICATION'
            ? 'A confirmation email is on the way. Click the button in that email to activate your account.'
            : loadTimeData.getString('braveAccountRowDescription')}
        </div>
      </div>
      ${this.state === 'VERIFICATION' ? nothing : html`
        <leo-button kind=${this.state === 'LOGGED_IN' ? 'outline' : 'filled'}
                    size="small"
                    @click=${this.onButtonClicked}>
          ${this.state === 'LOGGED_IN'
            ? loadTimeData.getString('braveAccountManageAccountButtonLabel')
            : loadTimeData.getString('braveAccountGetStartedButtonLabel')
          }
        </leo-button>`}
    </div>`
}
