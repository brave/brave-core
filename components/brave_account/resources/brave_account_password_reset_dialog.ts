/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import {
  ResetPasswordClientErrorCode,
  ResetPasswordError,
} from './brave_account.mojom-webui.js'
import { getCss } from './brave_account_password_reset_dialog.css.js'
import { getHtml } from './brave_account_password_reset_dialog.html.js'
import { showError } from './brave_account_common.js'

export class BraveAccountPasswordResetDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-password-reset-dialog'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      email: { type: String },
      isEmailValid: { type: Boolean },
    }
  }

  protected async onResetPasswordButtonClicked() {
    try {
      await this.browserProxy.authentication.resetPasswordVerifyInit(this.email)
    } catch (e) {
      let error: ResetPasswordError

      if (e && typeof e === 'object') {
        error = e as ResetPasswordError
      } else {
        console.error('Unexpected error:', e)
        error = {
          clientError: { errorCode: ResetPasswordClientErrorCode.kUnexpected },
        }
      }

      showError({ kind: 'resetPassword', details: error })
    }
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor email: string = ''
  protected accessor isEmailValid: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-password-reset-dialog': BraveAccountPasswordResetDialogElement
  }
}

customElements.define(
  BraveAccountPasswordResetDialogElement.is,
  BraveAccountPasswordResetDialogElement,
)
