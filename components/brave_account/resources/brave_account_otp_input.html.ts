/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountOtpInputElement } from './brave_account_otp_input.js'

// Note: disable iOS text assistance (spellcheck="false").
// It may perform replacement edits (for example smart punctuation),
// inserting invalid characters and replacing valid ones, corrupting the OTP.
// Example sequence:
//   beforeinput: insertText            "."
//   beforeinput: insertText            "."
//   beforeinput: deleteContentBackward
//   beforeinput: insertText            "…"
export function getHtml(this: BraveAccountOtpInputElement) {
  return html` ${this.getElementHtml()} `
}
