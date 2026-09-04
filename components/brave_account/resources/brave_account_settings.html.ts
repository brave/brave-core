/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_logged_in_row.js'
import './brave_account_logged_out_row.js'
import {
  AccountStateFieldTags,
  whichAccountState,
} from './brave_account.mojom-webui.js'
import { BraveAccountSettingsElement } from './brave_account_settings.js'

export function getHtml(this: BraveAccountSettingsElement) {
  return this.state === undefined ? nothing
    : whichAccountState(this.state) === AccountStateFieldTags.LOGGED_IN
      ? html`
        <brave-account-logged-in-row
            .browserProxy=${this.browserProxy}
            .initiatingServiceName=${this.initiatingServiceName}
            @open-brave-account-dialog=${this.onOpenBraveAccountDialog}
            .state=${this.state.loggedIn}>
        </brave-account-logged-in-row>`
      : html`
        <brave-account-logged-out-row
            .browserProxy=${this.browserProxy}
            .initiatingServiceName=${this.initiatingServiceName}
            @open-brave-account-dialog=${this.onOpenBraveAccountDialog}
            .state=${this.state.loggedOut}>
        </brave-account-logged-out-row>`
}
