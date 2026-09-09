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
      isConfirmed: { type: Boolean, state: true },
      isDeleting: { type: Boolean, state: true },
    }
  }

  protected onInput(detail: { value: string }) {
    this.isConfirmed = detail.value.trim() === this.confirmationKeyword
  }

  protected onCancelButtonClicked() {
    this.fire('close-dialog')
  }

  protected async onDeleteAccountButtonClicked() {
    if (this.isDeleting || !this.isConfirmed) return
    this.isDeleting = true

    try {
      await this.browserProxy.authentication.deleteAccount()
    } catch (e) {
      showError('deleteAccount', e)
    } finally {
      this.isDeleting = false
    }
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  private confirmationKeyword = loadTimeData.getString(
    BraveAccountStrings.BRAVE_ACCOUNT_DELETION_CONFIRMATION_KEYWORD,
  )

  protected accessor isConfirmed = false
  protected accessor isDeleting = false
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
