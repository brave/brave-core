/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { AccountState } from '../brave_account_row.mojom-webui.js'
import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl
} from './brave_account_browser_proxy.js'
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
      loading: { type: Boolean },
      state: { type: Object },
    }
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()
  protected accessor loading: boolean = true
  protected accessor state: AccountState

  override connectedCallback() {
    super.connectedCallback()

    this.loadInitialState()
    this.browserProxy.rowClientCallbackRouter.updateState.addListener(
      (state: AccountState) => { this.state = state; });
  }

  protected onLogOutButtonClicked() {
    this.browserProxy.authentication.logOut()
  }

  protected onResendConfirmationEmailButtonClicked() {
    this.browserProxy.authentication.resendConfirmationEmail()
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
    this.loading = false
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-row': SettingsBraveAccountRow
  }
}

customElements.define(SettingsBraveAccountRow.is, SettingsBraveAccountRow)
