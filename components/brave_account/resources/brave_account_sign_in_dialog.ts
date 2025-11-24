/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getCss } from './brave_account_sign_in_dialog.css.js'
import { getHtml } from './brave_account_sign_in_dialog.html.js'
import { Error, isEmailValid } from './brave_account_common.js'
import { LoginError, LoginErrorCode } from './brave_account.mojom-webui.js'

// @ts-expect-error
import { Login } from 'chrome://resources/brave/opaque_ke.bundle.js'

export class BraveAccountSignInDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-sign-in-dialog'
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
      password: { type: String },
    }
  }

  protected onEmailInput(detail: { value: string }) {
    this.email = detail.value.trim()
  }

  protected onPasswordInput(detail: { value: string }) {
    this.password = detail.value
  }

  // The reason this happens here (rather than in BraveAccountService) is that
  // both `login.start()` and `login.finish()` invoke the OPAQUE
  // protocol in our WASM (compiled from Rust), and so the flow must run in the
  // renderer to manage the transient cryptographic state — the service only
  // transports the two server round trips
  // (`loginInitialize`/`loginFinalize`). We'll revisit handling this
  // through Mojo in C++ if that proves practical.
  protected async onSignInButtonClicked() {
    try {
      const serializedKE1 = this.login.start(this.password)
      const { encryptedLoginToken, serializedKE2 } =
        await this.browserProxy.authentication.loginInitialize(
          this.email,
          serializedKE1,
        )
      const clientMac = this.login.finish(
        serializedKE2,
        this.password,
        this.email,
      )
      await this.browserProxy.authentication.loginFinalize(
        encryptedLoginToken,
        clientMac,
      )
      this.fire('close-dialog')
    } catch (error) {
      let details: LoginError

      if (error && typeof error === 'object') {
        details = error as LoginError
      } else if (typeof error === 'string') {
        details = {
          statusCode: null,
          errorCode: LoginErrorCode.kOpaqueError,
        }
      } else {
        console.error('Unexpected error:', error)
        details = { statusCode: null, errorCode: null }
      }

      this.fire('error-occurred', {
        flow: 'login',
        details,
      } satisfies Extract<Error, { flow: 'login' }>)
    }
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected login = new Login()

  protected accessor email: string = ''
  protected accessor password: string = ''

  protected get isEmailValid(): boolean {
    return isEmailValid(this.email)
  }

  protected get shouldShowEmailError(): boolean {
    return this.email.length !== 0 && !this.isEmailValid
  }

  protected get isPasswordValid(): boolean {
    return this.password.length !== 0
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-sign-in-dialog': BraveAccountSignInDialogElement
  }
}

customElements.define(
  BraveAccountSignInDialogElement.is,
  BraveAccountSignInDialogElement,
)
