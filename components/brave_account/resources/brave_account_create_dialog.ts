/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getCss } from './brave_account_create_dialog.css.js'
import { getHtml } from './brave_account_create_dialog.html.js'
import { Error, makeFocusHandler } from './brave_account_common.js'
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

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      email: { type: String },
      isCapsLockOn: { type: Boolean },
      isEmailValid: { type: Boolean },
      isPasswordInputFocused: { type: Boolean },
      isPasswordConfirmationInputFocused: { type: Boolean },
      isPasswordStrongEnough: { type: Boolean },
      password: { type: String },
      passwordConfirmation: { type: String },
    }
  }

  protected onPasswordInput(detail: { value: string }) {
    this.password = detail.value
  }

  protected onPasswordConfirmationInput(detail: { value: string }) {
    this.passwordConfirmation = detail.value
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
          this.browserProxy.getInitiatingServiceName(),
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

  // TODO(sszaloki): we should consider exporting `noChange`
  // from third_party/lit/v3_0/lit.ts instead, so that such
  // a workaround is not needed.
  protected getIconName() {
    if (this.passwordConfirmation.length !== 0) {
      this.icon =
        this.passwordConfirmation === this.password
          ? 'check-circle-filled'
          : 'warning-triangle-filled'
    }

    return this.icon
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected icon: string = 'warning-triangle-filled'
  protected accessor email: string = ''
  protected accessor isCapsLockOn: boolean = false
  protected accessor isEmailValid: boolean = false
  protected accessor isPasswordInputFocused: boolean = false
  protected accessor isPasswordConfirmationInputFocused: boolean = false
  protected accessor isPasswordStrongEnough: boolean = false
  protected accessor password: string = ''
  protected accessor passwordConfirmation: string = ''
  protected registration = new Registration()

  protected readonly passwordFocusHandler = makeFocusHandler(
    (f) => (this.isPasswordInputFocused = f),
  )
  protected readonly passwordConfirmationFocusHandler = makeFocusHandler(
    (f) => (this.isPasswordConfirmationInputFocused = f),
  )
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
