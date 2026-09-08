/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { BraveAccountStrings } from './brave_components_webui_strings.js'
import { getCss } from './brave_account_deletion_dialog.css.js'
import { getHtml } from './brave_account_deletion_dialog.html.js'
import { showError } from './brave_account_shared.js'

export class BraveAccountDeletionDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-deletion-dialog'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      confirmation: { type: String, state: true },
      isSubmitting: { type: Boolean, state: true },
    }
  }

  // Takes the keyword as a placeholder, so the label cannot go out of sync
  // with the word `onInput` compares against.
  protected getConfirmationLabel() {
    return loadTimeData.getStringF(
      BraveAccountStrings.BRAVE_ACCOUNT_DELETION_CONFIRMATION_LABEL,
      this.confirmationKeyword,
    )
  }

  protected onInput(detail: { value: string }) {
    this.confirmation = detail.value.trim()
  }

  protected isConfirmed() {
    return this.confirmation === this.confirmationKeyword
  }

  // An empty field is not an error yet - it is the starting state.
  protected hasError() {
    return this.confirmation.length !== 0 && !this.isConfirmed()
  }

  protected async onDeleteAccountButtonClicked() {
    if (this.isSubmitting) return
    this.isSubmitting = true

    try {
      await this.browserProxy.authentication.deleteAccount()
    } catch (e) {
      // Reset only on failure: on success the account leaves the logged-in
      // state and the page closes, so re-enabling would briefly offer a
      // second deletion mid-teardown.
      showError('deleteAccount', e)
      this.isSubmitting = false
    }
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  private confirmationKeyword = loadTimeData.getString(
    BraveAccountStrings.BRAVE_ACCOUNT_DELETION_CONFIRMATION_KEYWORD,
  )

  protected accessor confirmation = ''
  protected accessor isSubmitting = false
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-deletion-dialog': BraveAccountDeletionDialogElement
  }
}

customElements.define(
  BraveAccountDeletionDialogElement.is,
  BraveAccountDeletionDialogElement,
)
