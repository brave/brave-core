/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement, css, html } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getCss } from './brave_account_create_dialog.css.js'
import { getHtml } from './brave_account_create_dialog.html.js'
import { Error, isEmailValid } from './brave_account_common.js'
import {
  RegisterError,
  RegisterErrorCode,
} from './brave_account.mojom-webui.js'

// @ts-expect-error
import { Registration } from 'chrome://resources/brave/opaque_ke.bundle.js'

class PasswordStrengthMeter extends CrLitElement {
  static get is() {
    return 'password-strength-meter'
  }

  static override get styles() {
    return css`
      :host {
        align-items: center;
        display: flex;
        gap: var(--leo-spacing-m);
        justify-content: space-between;
        width: 100%;
      }

      :host([category='Weak']) {
        --primary-color: var(--leo-color-systemfeedback-error-icon);
        --secondary-color: var(--leo-color-systemfeedback-error-background);
      }

      :host([category='Medium']) {
        --primary-color: var(--leo-color-systemfeedback-warning-icon);
        --secondary-color: var(--leo-color-systemfeedback-warning-background);
      }

      :host([category='Strong']) {
        --primary-color: var(--leo-color-systemfeedback-success-icon);
        --secondary-color: var(--leo-color-systemfeedback-success-background);
      }

      .bar {
        background-color: var(--secondary-color);
        border-radius: var(--leo-radius-m);
        flex-grow: 1;
        height: 4px;
        transition: 750ms;
      }

      .strength {
        background-color: var(--primary-color);
        border-radius: var(--leo-radius-m);
        height: 100%;
        transition: 750ms;
        width: calc(1% * var(--strength));
      }

      .text {
        color: var(--primary-color);
        font: var(--leo-font-small-regular);
        transition: 750ms;
      }
    `
  }

  override render() {
    return html`
      <div class="bar">
        <div
          class="strength"
          style="--strength: ${this.strength}"
        ></div>
      </div>
      <div class="text">
        ${loadTimeData.getString(
          `braveAccountPasswordStrengthMeter${this.category}`,
        )}
      </div>
    `
  }

  static override get properties() {
    return {
      category: { type: String, reflect: true },
      strength: { type: Number },
    }
  }

  override updated(changedProperties: Map<PropertyKey, unknown>) {
    if (changedProperties.has('strength')) {
      this.category =
        this.strength < 60 ? 'Weak' : this.strength < 100 ? 'Medium' : 'Strong'
    }
  }

  protected accessor category: 'Weak' | 'Medium' | 'Strong' = 'Weak'
  protected accessor strength: number = 0
}

declare global {
  interface HTMLElementTagNameMap {
    'password-strength-meter': PasswordStrengthMeter
  }
}

customElements.define(PasswordStrengthMeter.is, PasswordStrengthMeter)

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
      isCheckboxChecked: { type: Boolean },
      isEmailBraveAlias: { type: Boolean },
      isEmailValid: { type: Boolean },
      password: { type: String },
      passwordConfirmation: { type: String },
      passwordStrength: { type: Number },
    }
  }

  protected onEmailInput(detail: { value: string }) {
    this.email = detail.value.trim()
    this.isEmailValid = isEmailValid(this.email)
    this.isEmailBraveAlias = /@bravealias\.com$/i.test(this.email)
  }

  protected onPasswordInput(detail: { value: string }) {
    this.password = detail.value
    this.browserProxy.password_strength_meter
      .getPasswordStrength(this.password)
      .then(
        (value: { strength: number }) =>
          (this.passwordStrength = value.strength),
      )
  }

  protected onConfirmPasswordInput(detail: { value: string }) {
    this.passwordConfirmation = detail.value
  }

  protected onCheckboxChanged(detail: { checked: boolean }) {
    this.isCheckboxChecked = detail.checked
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
      } satisfies Error<'register'>)
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
  protected accessor isCheckboxChecked: boolean = false
  protected accessor isEmailBraveAlias: boolean = false
  protected accessor isEmailValid: boolean = false
  protected accessor password: string = ''
  protected accessor passwordConfirmation: string = ''
  protected accessor passwordStrength: number = 0
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
