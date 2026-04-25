/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountPasswordStrengthMeterElement } from './brave_account_password_strength_meter.js'

export function getHtml(this: BraveAccountPasswordStrengthMeterElement) {
  return html`<!--_html_template_start_-->
    <div class="bar">
      <div
        class="strength"
        style="--strength: ${this.strength}"
      ></div>
    </div>
    <div class="text">
      ${this.category === 'Weak'
        ? '$i18n{BRAVE_ACCOUNT_PASSWORD_STRENGTH_METER_WEAK}'
        : this.category === 'Medium'
          ? '$i18n{BRAVE_ACCOUNT_PASSWORD_STRENGTH_METER_MEDIUM}'
          : '$i18n{BRAVE_ACCOUNT_PASSWORD_STRENGTH_METER_STRONG}'}
    </div>
    <!--_html_template_end_-->`
}
