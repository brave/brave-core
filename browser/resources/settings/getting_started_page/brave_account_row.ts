/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement, html, render } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

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

export class SettingsBraveAccountRow extends CrLitElement {
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
  private toastContainer: HTMLDivElement | undefined = undefined

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
    let details: ResendConfirmationEmailError | undefined

    try {
      await this.browserProxy.authentication.resendConfirmationEmail()
    } catch (error) {
      if (error && typeof error === 'object') {
        details = error as ResendConfirmationEmailError
      } else {
        console.error('Unexpected error:', error)
        details = { netErrorOrHttpStatus: null, errorCode: null }
      }
    }

    this.showToast(details)
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

  private showToast(details: ResendConfirmationEmailError | undefined) {
    const hideToast = (container: HTMLDivElement | undefined) => {
      container?.remove()
    }

    hideToast(this.toastContainer)
    this.toastContainer = document.createElement('div')
    document.body.appendChild(this.toastContainer)

    // Capture the container for this specific toast.
    const container = this.toastContainer
    render(html`
      <leo-alert type="${details ? 'error' : 'success'}"
                 isToast="true"
                 style="left: 50%;
                        position: fixed;
                        top: var(--leo-spacing-5xl);
                        transform: translateX(-50%);">
        ${details
          ? this.getErrorMessage(details)
          : loadTimeData.getString(
                'braveAccountResendConfirmationEmailSuccess')}
        <leo-button slot="content-after"
                    fab="true"
                    kind="plain-faint"
                    size="tiny"
                    style="align-items: center;
                           display: flex;
                           height: 100%;"
                    @click=${() => hideToast(container)}>
          <leo-icon name="close"></leo-icon>
        </leo-button>
      </leo-alert>
    `, container)

    setTimeout(() => hideToast(container), 30000)
  }

  private getErrorMessage(details: ResendConfirmationEmailError): string {
    const ERROR_STRINGS: Partial<
      Record<ResendConfirmationEmailErrorCode, string>
    > = {
      [ResendConfirmationEmailErrorCode.kMaximumEmailSendAttemptsExceeded]:
        loadTimeData.getString(
            'braveAccountResendConfirmationEmailMaximumSendAttemptsExceeded'),
      [ResendConfirmationEmailErrorCode.kEmailAlreadyVerified]:
        loadTimeData.getString(
            'braveAccountResendConfirmationEmailAlreadyVerified'),
    }

    const { netErrorOrHttpStatus, errorCode } = details

    if (netErrorOrHttpStatus == null) {
      // client-side error
      return loadTimeData.getStringF(
          'braveAccountClientError',
          errorCode != null
            ? ` (${loadTimeData.getString('braveAccountError')}=${errorCode})`
            : '',
      )
    }

    // server-side error
    return (
      (errorCode != null ? ERROR_STRINGS[errorCode] : null)
      ?? loadTimeData.getStringF(
        'braveAccountServerError',
        netErrorOrHttpStatus,
        errorCode != null
          ? `, ${loadTimeData.getString('braveAccountError')}=${errorCode}`
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
