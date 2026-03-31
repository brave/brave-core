/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getHtml } from './brave_account_create_dialog.html.js'
import { Error } from './brave_account_common.js'
import {
  RegisterError,
  RegisterErrorCode,
} from './brave_account.mojom-webui.js'

// @ts-expect-error
import { Registration } from 'chrome://resources/brave/opaque_ke.bundle.js'

export class BraveAccountCreateDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-create-dialog'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      email: { type: String },
      isCapsLockOn: { type: Boolean },
      isEmailValid: { type: Boolean },
      isPasswordStrongEnough: { type: Boolean },
      isPasswordValid: { type: Boolean },
      password: { type: String },
      passwordConfirmation: { type: String },
    }
  }

  // The reason this happens here (rather than in BraveAccountService) is that
  // both `registration.start()` and `registration.finish()` invoke the OPAQUE
  // protocol in our WASM (compiled from Rust), and so the flow must run in the
  // renderer to manage the transient cryptographic state — the service only
  // transports the two server round trips
  // (`registerInitialize`/`registerFinalize`). We'll revisit handling this
  // through Mojo in C++ if that proves practical.
  protected async onCreateAccountButtonClicked() {
    try {
      const blindedMessage = this.registration.start(this.password)
      const { encryptedVerificationToken, serializedResponse } =
        await this.browserProxy.authentication.registerInitialize(
          this.browserProxy.getInitiatingService(),
          this.email,
          blindedMessage,
        )
      const serializedRecord = this.registration.finish(
        serializedResponse,
        this.password,
        this.email,
      )
      await this.browserProxy.authentication.registerFinalize(
        encryptedVerificationToken,
        serializedRecord,
      )
      this.fire('close-dialog')
    } catch (error) {
      let details: RegisterError

      if (error && typeof error === 'object') {
        details = error as RegisterError
      } else if (typeof error === 'string') {
        details = {
          statusCode: null,
          errorCode: RegisterErrorCode.kOpaqueError,
        }
      } else {
        console.error('Unexpected error:', error)
        details = { statusCode: null, errorCode: null }
      }

      this.fire('error-occurred', {
        flow: 'register',
        details,
      } satisfies Extract<Error, { flow: 'register' }>)
    }
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor email: string = ''
  protected accessor isCapsLockOn: boolean = false
  protected accessor isEmailValid: boolean = false
  protected accessor isPasswordStrongEnough: boolean = false
  protected accessor isPasswordValid: boolean = false
  protected accessor password: string = ''
  protected accessor passwordConfirmation: string = ''
  protected registration = new Registration()
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-create-dialog': BraveAccountCreateDialogElement
  }
}

customElements.define(
  BraveAccountCreateDialogElement.is,
  BraveAccountCreateDialogElement,
)
