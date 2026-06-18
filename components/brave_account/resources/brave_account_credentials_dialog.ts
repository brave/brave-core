/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert, assertNotReached } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getHtml } from './brave_account_credentials_dialog.html.js'
import {
  LoggedOutVerification,
  LoggedOutVerificationIntent,
  RegisterClientErrorCode,
  RegisterError,
  ResetPasswordClientErrorCode,
  ResetPasswordError,
} from './brave_account.mojom-webui.js'
import { showError } from './brave_account_common.js'

// @ts-expect-error: no type definitions are generated for opaque_ke.bundle.js
import { Registration } from 'chrome://resources/brave/opaque_ke.bundle.js'

export class BraveAccountCredentialsDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-credentials-dialog'
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
      isSubmitting: { type: Boolean, state: true },
      password: { type: String },
      passwordConfirmation: { type: String },
      verification: { type: Object },
    }
  }

  // The reason this happens here (rather than in BraveAccountService) is that
  // both `registration.start()` and `registration.finish()` invoke the OPAQUE
  // protocol in our WASM (compiled from Rust), and so the flow must run in the
  // renderer to manage the transient cryptographic state — the service only
  // transports the two server round trips. We'll revisit handling this
  // through Mojo in C++ if that proves practical.
  protected async onSubmitButtonClicked() {
    if (this.isSubmitting) return
    this.isSubmitting = true

    if (!this.verification) {
      await this.register()
    } else {
      switch (this.verification.intent) {
        case LoggedOutVerificationIntent.kResetPassword:
          await this.resetPassword()
          break
        default:
          // kRegistration never arrives with a verification - that's the
          // registration path above, which has no verification at all.
          assertNotReached(`Unexpected intent: ${this.verification.intent}!`)
      }
    }

    this.isSubmitting = false
  }

  private async register() {
    try {
      const blindedMessage = this.registration.start(this.password)
      const { encryptedVerificationToken, serializedResponse } =
        await this.browserProxy.authentication.registerInitialize(
          this.browserProxy.getInitiatingService(),
          this.getEmail(),
          blindedMessage,
        )
      const serializedRecord = this.registration.finish(
        serializedResponse,
        this.password,
        this.getEmail(),
      )
      await this.browserProxy.authentication.registerFinalize(
        encryptedVerificationToken,
        serializedRecord,
      )
    } catch (e) {
      let error: RegisterError

      if (e && typeof e === 'object') {
        error = e as RegisterError
      } else if (typeof e === 'string') {
        error = {
          clientError: { errorCode: RegisterClientErrorCode.kOpaqueError },
        }
      } else {
        console.error('Unexpected error:', e)
        error = {
          clientError: { errorCode: RegisterClientErrorCode.kUnexpected },
        }
      }

      showError({ kind: 'register', details: error })
    }
  }

  private async resetPassword() {
    try {
      const blindedMessage = this.registration.start(this.password)
      const { serializedResponse } =
        await this.browserProxy.authentication.resetPasswordPasswordInit(
          blindedMessage,
        )
      const serializedRecord = this.registration.finish(
        serializedResponse,
        this.password,
        this.getEmail(),
      )
      await this.browserProxy.authentication.resetPasswordPasswordFinalize(
        serializedRecord,
        this.getEmail(),
      )
    } catch (e) {
      let error: ResetPasswordError

      if (e && typeof e === 'object') {
        error = e as ResetPasswordError
      } else if (typeof e === 'string') {
        error = {
          clientError: { errorCode: ResetPasswordClientErrorCode.kOpaqueError },
        }
      } else {
        console.error('Unexpected error:', e)
        error = {
          clientError: { errorCode: ResetPasswordClientErrorCode.kUnexpected },
        }
      }

      showError({ kind: 'resetPassword', details: error })
    }
  }

  private getEmail(): string {
    const email = this.verification?.verifiedEmail ?? this.email
    assert(email)
    return email
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor email: string = ''
  protected accessor isCapsLockOn: boolean = false
  protected accessor isEmailValid: boolean = false
  protected accessor isPasswordStrongEnough: boolean = false
  protected accessor isPasswordValid: boolean = false
  protected accessor isSubmitting: boolean = false
  protected accessor password: string = ''
  protected accessor passwordConfirmation: string = ''
  protected accessor verification: LoggedOutVerification | undefined = undefined
  protected registration = new Registration()
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-credentials-dialog': BraveAccountCredentialsDialogElement
  }
}

customElements.define(
  BraveAccountCredentialsDialogElement.is,
  BraveAccountCredentialsDialogElement,
)
