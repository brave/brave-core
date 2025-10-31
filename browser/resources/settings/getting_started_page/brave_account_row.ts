/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl
} from './brave_account_browser_proxy.js'
import { getCss } from './brave_account_row.css.js'
import { getHtml } from './brave_account_row.html.js'

export type State =
  | 'LOGGED_OUT' | 'VERIFICATION' | 'LOGGED_IN'

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
      state: { type: Object, reflect: true },
    }
  }

  protected onButtonClicked() {
    this.browserProxy.handler.openDialog()
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()
  protected accessor state: State = 'LOGGED_OUT'

  override connectedCallback() {
    super.connectedCallback()
    this.browserProxy.callbackRouter.onVerificationTokenChanged.addListener(
      () => { this.state = 'VERIFICATION'; });
    this.browserProxy.callbackRouter.onAuthenticationTokenChanged.addListener(
      () => { this.state = 'LOGGED_IN'; });
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-row': SettingsBraveAccountRow
  }
}

customElements.define(SettingsBraveAccountRow.is, SettingsBraveAccountRow)
