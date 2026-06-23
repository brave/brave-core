/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-logged-out-row': BraveAccountLoggedOutRowElement
  }
}

customElements.define(
  BraveAccountLoggedOutRowElement.is, BraveAccountLoggedOutRowElement)
