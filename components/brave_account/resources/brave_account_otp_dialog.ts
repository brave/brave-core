/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { Error } from './brave_account_common.js'
import { getHtml } from './brave_account_otp_dialog.html.js'
import { RegisterError } from './brave_account.mojom-webui.js'

export class BraveAccountOtpDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-otp-dialog'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      code: { type: String },
      isCodeValid: { type: Boolean },
    }
  }

  protected async onConfirmCodeButtonClicked() {
    try {
      await this.browserProxy.authentication.registerVerify(this.code)
    } catch (error) {
      let details: RegisterError

      if (error && typeof error === 'object') {
        details = error as RegisterError
      } else {
        console.error('Unexpected error:', error)
        details = { netErrorOrHttpStatus: null, errorCode: null }
      }

      this.fire('error-occurred', {
        flow: 'register',
        details,
      } satisfies Extract<Error, { flow: 'register' }>)
    }
  }

  protected onResendEmailCodeButtonClicked() {}

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor code = ''
  protected accessor isCodeValid = false
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-otp-dialog': BraveAccountOtpDialogElement
  }
}

customElements.define(
  BraveAccountOtpDialogElement.is,
  BraveAccountOtpDialogElement,
)
