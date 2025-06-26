/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

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
          ${this.signedIn
            ? 'John Doe'
            : loadTimeData.getString('braveAccountRowTitle')}
        </div>
        <div class="description">
          ${this.signedIn
            ? 'johndoe@gmail.com'
            : loadTimeData.getString('braveAccountRowDescription')}
        </div>
      </div>
      <leo-button kind=${this.signedIn ? 'outline' : 'filled'}
                  size="small"
                  @click=${this.onButtonClicked}>
        ${this.signedIn
          ? loadTimeData.getString('braveAccountManageAccountButtonLabel')
          : loadTimeData.getString('braveAccountGetStartedButtonLabel')
        }
      </leo-button>
    </div>`
}
