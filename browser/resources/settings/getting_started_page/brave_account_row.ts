/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'
// @ts-expect-error
import { leoShowAlert } from '//resources/brave/leo.bundle.js'

import { AccountState } from '../brave_account_row.mojom-webui.js'
import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl
} from './brave_account_browser_proxy.js'
import {
  ResendConfirmationEmailError,
  ResendConfirmationEmailErrorCode,
} from '../brave_account.mojom-webui.js'
import { getCss } from './brave_account_row.css.js'
import { getHtml } from './brave_account_row.html.js'

export class SettingsBraveAccountRow extends I18nMixinLit(CrLitElement) {
  static get is() {
    return 'settings-brave-account-row'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      state: { type: Object },
    }
  }

  protected accessor state: AccountState | undefined = undefined

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  override connectedCallback() {
    super.connectedCallback()

    this.loadInitialState()
    this.browserProxy.rowClientCallbackRouter.updateState.addListener(
      (state: AccountState) => { this.state = state; });
  }

  protected onLogOutButtonClicked() {
    this.browserProxy.authentication.logOut()
  }

  protected async onResendConfirmationEmailButtonClicked() {
    let error: ResendConfirmationEmailError | undefined

    try {
      await this.browserProxy.authentication.resendConfirmationEmail()
    } catch (e) {
      if (e && typeof e === 'object') {
        error = e as ResendConfirmationEmailError
      } else {
        console.error('Unexpected error:', e)
        error = { netErrorOrHttpStatus: null, errorCode: null }
      }
    }

    leoShowAlert({
      type: error ? 'error' : 'success',
      content: error
        ? this.getErrorMessage(error)
        : this.i18n('braveAccountResendConfirmationEmailSuccess')
    }, 30000)
  }

  protected onCancelRegistrationButtonClicked() {
    this.browserProxy.authentication.cancelRegistration()
  }

  protected onGetStartedButtonClicked() {
    this.browserProxy.rowHandler.openDialog()
  }

  private async loadInitialState() {
    const { state } = await this.browserProxy.rowHandler.getAccountState()
    this.state = state
  }

  private getErrorMessage(details: ResendConfirmationEmailError): string {
    const ERROR_STRINGS: Partial<
      Record<ResendConfirmationEmailErrorCode, string>
    > = {
      [ResendConfirmationEmailErrorCode.kMaximumEmailSendAttemptsExceeded]:
        this.i18n(
            'braveAccountResendConfirmationEmailMaximumSendAttemptsExceeded'),
      [ResendConfirmationEmailErrorCode.kEmailAlreadyVerified]:
        this.i18n(
            'braveAccountResendConfirmationEmailAlreadyVerified'),
    }

    const { netErrorOrHttpStatus, errorCode } = details

    if (netErrorOrHttpStatus == null) {
      // client-side error
      return this.i18n(
          'braveAccountClientError',
          errorCode != null
            ? ` (${this.i18n('braveAccountError')}=${errorCode})`
            : '',
      )
    }

    // server-side error
    return (
      (errorCode != null ? ERROR_STRINGS[errorCode] : null)
      ?? this.i18n(
        'braveAccountServerError',
        netErrorOrHttpStatus,
        errorCode != null
          ? `, ${this.i18n('braveAccountError')}=${errorCode}`
          : '',
      )
    )
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-row': SettingsBraveAccountRow
  }
}

customElements.define(SettingsBraveAccountRow.is, SettingsBraveAccountRow)
