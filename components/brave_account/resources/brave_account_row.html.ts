/* Copyright (c) 2026 The Brave Authors. All rights reserved.
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
import { BraveAccountRowElement } from './brave_account_row.js'

export function getHtml(this: BraveAccountRowElement) {
  return this.state === undefined
    ? nothing
    : whichAccountState(this.state) === AccountStateFieldTags.LOGGED_IN
      ? html` <brave-account-logged-in-row
          .browserProxy=${this.browserProxy}
          .initiatingServiceName=${this.initiatingServiceName}
          .state=${this.state.loggedIn}
          @open-brave-account-dialog=${this.onOpenBraveAccountDialog}
        >
        </brave-account-logged-in-row>`
      : html` <brave-account-logged-out-row
          .browserProxy=${this.browserProxy}
          .initiatingServiceName=${this.initiatingServiceName}
          .state=${this.state.loggedOut}
          @open-brave-account-dialog=${this.onOpenBraveAccountDialog}
        >
        </brave-account-logged-out-row>`
}
