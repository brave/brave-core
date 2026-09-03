/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'

import { BraveAccountBrowserProxy } from './brave_account_browser_proxy.js'
import { VerificationIntent } from '../brave_account.mojom-webui.js'
import {
  ResendVerificationEmailClientErrorCode,
  ResendVerificationEmailError,
} from '../resend_verification_email.mojom-webui.js'
import {
  showResendVerificationEmailResult,
  toFlowError,
} from '../brave_account_shared.js'

// Shared by the logged-out and logged-in rows, which differ only in their
// verification intent type (`Intent`) and how it is tagged into a
// `VerificationIntent` (logged-out vs logged-in). `Intent` is the bare
// per-state intent enum carried by `state.verification`.
export abstract class BraveAccountRowBaseElement<
  Intent,
  State extends { verification: { intent: Intent } | null },
> extends I18nMixinLit(CrLitElement) {
  static override get properties() {
    return {
      browserProxy: { type: Object },
      initiatingServiceName: { type: String },
      state: { type: Object },
    }
  }

  accessor browserProxy!: BraveAccountBrowserProxy
  protected accessor initiatingServiceName = ''
  // `& object` is only here to satisfy the @webui-eslint/lit-property-accessor
  // lint rule, which expects Object reactive properties to be typed as objects.
  // The actual shape of `state` is defined by the `State` constraint above.
  protected accessor state!: State & object

  private isResendingConfirmationEmail = false

  // Tags the bare per-state intent into the union the service expects.
  protected abstract makeVerificationIntent(intent: Intent): VerificationIntent

  protected async onResendConfirmationEmailLinkClicked(
        e: CustomEvent<{event: Event}>) {
    e.detail.event.preventDefault()

    if (this.isResendingConfirmationEmail) return
    this.isResendingConfirmationEmail = true

    let error: ResendVerificationEmailError | undefined

    assert(this.state.verification)
    try {
      await this.browserProxy.authentication.resendVerificationEmail(
        this.makeVerificationIntent(this.state.verification.intent))
    } catch (e) {
      error = toFlowError<
        ResendVerificationEmailError,
        ResendVerificationEmailClientErrorCode
      >(e, ResendVerificationEmailClientErrorCode.kUnexpected)
    }

    showResendVerificationEmailResult(error, 30000)

    this.isResendingConfirmationEmail = false
  }

  protected onCancelVerificationButtonClicked() {
    assert(this.state.verification)
    this.browserProxy.authentication.cancelVerification(
      this.makeVerificationIntent(this.state.verification.intent))
  }

  protected openBraveAccountDialog() {
    this.browserProxy.rowHandler.openDialog(this.initiatingServiceName)
  }

}
