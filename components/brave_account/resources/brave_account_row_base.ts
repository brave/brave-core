/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'
import { parseHtmlSubset } from '//resources/js/parse_html_subset.js'

import { BraveAccountRowBrowserProxy } from './brave_account_row_browser_proxy.js'
import { VerificationIntent } from './brave_account.mojom-webui.js'
import { showError, showSuccess } from './brave_account_shared.js'

// Shared by the logged-out and logged-in rows, which differ only in their
// verification intent type (`Intent`) and how it is tagged into a
// `VerificationIntent` (logged-out vs logged-in). `Intent` is the bare
// per-state intent enum carried by `state.verification`.
export abstract class BraveAccountRowBaseElement<
  Intent,
  State extends { verification: { intent: Intent } | null },
> extends CrLitElement {
  // cr_elements only builds i18n_mixin_lit.ts when !is_ios, but these rows are
  // also served as a page there, so I18nMixinLit is not available. This is the
  // one method of it the rows use, over loadTimeData directly.
  //
  // Its i18nAdvanced() counterpart is deliberately absent: it goes through
  // sanitizeInnerHtml(), whose iOS build asserts that window.trustedTypes does
  // not exist - which modern WebKit provides, so it always fires there.
  private i18nRaw(id: string, ...varArgs: Array<string | number>) {
    return varArgs.length === 0
      ? loadTimeData.getString(id)
      : loadTimeData.getStringF(id, ...varArgs)
  }

  protected i18n(id: string, ...varArgs: Array<string | number>) {
    return parseHtmlSubset(
      `<b>${this.i18nRaw(id, ...varArgs)}</b>`).firstChild!.textContent!
  }

  static override get properties() {
    return {
      browserProxy: { type: Object },
      initiatingServiceName: { type: String },
      state: { type: Object },
    }
  }

  accessor browserProxy!: BraveAccountRowBrowserProxy
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

    assert(this.state.verification)
    try {
      await this.browserProxy.authentication.resendVerificationEmail(
        this.makeVerificationIntent(this.state.verification.intent))
      showSuccess('resendVerificationEmail', { durationMs: 30000 })
    } catch (e) {
      showError('resendVerificationEmail', e, { durationMs: 30000 })
    }

    this.isResendingConfirmationEmail = false
  }

  protected onCancelVerificationButtonClicked() {
    assert(this.state.verification)
    this.browserProxy.authentication.cancelVerification(
      this.makeVerificationIntent(this.state.verification.intent))
  }

  // How authentication is entered is the host's decision, not the row's:
  // desktop opens a ConstrainedWebDialog over brave://settings, while iOS
  // presents the flows over the page serving the rows. So the rows only
  // announce the intent and let their mount act on it.
  protected openBraveAccountDialog() {
    this.fire('open-brave-account-dialog', {
      initiatingServiceName: this.initiatingServiceName,
    })
  }

}
