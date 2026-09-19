/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { BraveAccountRowBrowserProxy } from './brave_account_row_browser_proxy.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'
import { DialogMode, VerificationIntent } from './brave_account.mojom-webui.js'
import { showError, showSuccess } from './brave_account_shared.js'

// Shared by the logged-out and logged-in rows, which differ only in their
// verification intent type (`Intent`) and how it is tagged into a
// `VerificationIntent` (logged-out vs logged-in). `Intent` is the bare
// per-state intent enum carried by `state.verification`.
export abstract class BraveAccountRowBaseElement<
  Intent,
  State extends { verification: { intent: Intent } | null },
> extends CrLitElement {
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

  // Opening sentence of the pending-verification description. Differs per row
  // and per intent, so each row supplies its own.
  protected abstract get verificationIntentDescription(): string

  // The pending-verification description ends with a sentence that wraps its
  // `resend` link text in `<a>` tags, so that the link text is translated in
  // context rather than as a standalone message. The tags are only used to
  // locate the link text - they are never parsed as HTML.
  protected getVerificationDescription() {
    const [beforeLink, linkLabel, afterLink] = loadTimeData
      .getString(
        BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_3,
      )
      .split(/<a>|<\/a>/)

    return {
      beforeLink: [
        this.verificationIntentDescription,
        loadTimeData.getString(BraveAccountSettingsStrings
          .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_2),
        beforeLink,
      ].join(' '),
      linkLabel,
      afterLink,
    }
  }

  protected async onResendConfirmationEmailLinkClicked() {
    if (this.isResendingConfirmationEmail) return
    this.isResendingConfirmationEmail = true

    assert(this.state.verification)
    try {
      await this.browserProxy.authentication.resendVerificationEmail(
        this.makeVerificationIntent(this.state.verification.intent),
      )
      showSuccess('resendVerificationEmail', { durationMs: 30000 })
    } catch (e) {
      showError('resendVerificationEmail', e, { durationMs: 30000 })
    }

    this.isResendingConfirmationEmail = false
  }

  protected onCancelVerificationButtonClicked() {
    assert(this.state.verification)
    this.browserProxy.authentication.cancelVerification(
      this.makeVerificationIntent(this.state.verification.intent),
    )
  }

  protected openDialogInDefaultMode() {
    this.openDialog(DialogMode.kDefault)
  }

  protected openDialogInAccountDeletionMode() {
    this.openDialog(DialogMode.kAccountDeletion)
  }

  // How the dialog is opened is the host's decision, not the row's, so the
  // rows only announce the intent and let their mount act on it.
  private openDialog(dialogMode: DialogMode) {
    this.fire('open-brave-account-dialog', {
      initiatingServiceName: this.initiatingServiceName,
      dialogMode,
    })
  }
}
