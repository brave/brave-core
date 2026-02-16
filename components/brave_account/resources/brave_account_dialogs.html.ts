/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_create_dialog.js'
import './brave_account_entry_dialog.js'
import './brave_account_error_dialog.js'
import './brave_account_forgot_password_dialog.js'
import './brave_account_otp_dialog.js'
import './brave_account_sign_in_dialog.js'
import { BraveAccountDialogsElement } from './brave_account_dialogs.js'

export function getHtml(this: BraveAccountDialogsElement) {
  // clang-format off
  return html` ${this.getDialogHtml()} `
  // clang-format on
}
