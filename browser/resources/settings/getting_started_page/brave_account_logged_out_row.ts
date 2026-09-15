/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { BraveAccountSettingsStrings } from '../brave_components_webui_strings.js'
import {
  LoggedOutState,
  LoggedOutVerificationIntent,
  VerificationIntent,
} from '../brave_account.mojom-webui.js'
import { BraveAccountRowBaseElement } from './brave_account_row_base.js'
import { getCss } from './brave_account_row_common.css.js'
import { getHtml } from './brave_account_logged_out_row.html.js'

export class BraveAccountLoggedOutRowElement extends
    BraveAccountRowBaseElement<LoggedOutVerificationIntent, LoggedOutState> {
  static get is() {
    return 'brave-account-logged-out-row'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  protected override makeVerificationIntent(
        intent: LoggedOutVerificationIntent): VerificationIntent {
    return { loggedOutIntent: intent }
  }

  protected override get verificationIntentDescription() {
    assert(this.state.verification)
    return this.i18n(
      this.state.verification.intent
          === LoggedOutVerificationIntent.kResetPassword
        ? BraveAccountSettingsStrings
            .SETTINGS_BRAVE_ACCOUNT_RESET_PASSWORD_ROW_DESCRIPTION_1
        : BraveAccountSettingsStrings
            .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_1)
  }

  // The logged-out description wraps its `Learn more` link text in `<a>` tags,
  // so that the link text is translated in context rather than as a standalone
  // message. The tags are only used to locate the link text - they are never
  // parsed as HTML.
  protected getLoggedOutDescription() {
    const [beforeLink, linkLabel, afterLink] = loadTimeData.getString(
      BraveAccountSettingsStrings
        .SETTINGS_BRAVE_ACCOUNT_LOGGED_OUT_ROW_DESCRIPTION)
      .split(/<a>|<\/a>/)

    return { beforeLink, linkLabel, afterLink }
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-logged-out-row': BraveAccountLoggedOutRowElement
  }
}

customElements.define(
  BraveAccountLoggedOutRowElement.is, BraveAccountLoggedOutRowElement)
