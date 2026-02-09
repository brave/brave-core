/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountPasswordIconsElement } from './brave_account_password_icons.js'

export function getHtml(this: BraveAccountPasswordIconsElement) {
  const showCapsLock =
    this.isCapsLockOn && this.isInputFocused && !this.isPasswordVisible

  return html`<!--_html_template_start_-->
    ${showCapsLock
      ? html`<leo-tooltip>
          <div slot="content">$i18n{BRAVE_ACCOUNT_CAPS_LOCK_ON}</div>
          <leo-icon name="caps-lock"></leo-icon>
        </leo-tooltip>`
      : nothing}
    <leo-icon
      name="${this.isPasswordVisible ? 'eye-on' : 'eye-off'}"
      @click=${this.onEyeIconClicked}
    ></leo-icon>
    <!--_html_template_end_-->`
}
