/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountRowBrowserProxy,
  BraveAccountRowBrowserProxyImpl
} from '../brave_account_row_browser_proxy.js'
import { AccountState } from '../brave_account.mojom-webui.js'
import { getHtml } from './brave_account_row.html.js'

export class SettingsBraveAccountRowElement extends CrLitElement {
  static get is() {
    return 'settings-brave-account-row'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      browserProxy: { type: Object },
      initiatingServiceName: { type: String },
      state: { type: Object },
    }
  }

  protected accessor browserProxy: BraveAccountRowBrowserProxy =
    new BraveAccountRowBrowserProxyImpl()
  protected accessor initiatingServiceName = ''
  protected accessor state: AccountState | undefined = undefined

  private accountStateListenerId: number | null = null

  // The rows live here in brave://settings rather than in the Brave Account
  // WebUI, so this asks the browser to open the flows in a ConstrainedWebDialog
  // over the page.
  protected onOpenBraveAccountDialog(
        e: CustomEvent<{ initiatingServiceName: string }>) {
    this.browserProxy.dialogController.openDialog(e.detail.initiatingServiceName)
  }

  override connectedCallback() {
    super.connectedCallback()

    this.accountStateListenerId =
      this.browserProxy.authenticationObserverCallbackRouter
        .onAccountStateChanged
        .addListener((state: AccountState) => {
          this.state = state
        })
  }

  override disconnectedCallback() {
    super.disconnectedCallback()

    assert(this.accountStateListenerId)
    this.browserProxy.authenticationObserverCallbackRouter.removeListener(
      this.accountStateListenerId)
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-row': SettingsBraveAccountRowElement
  }
}

customElements.define(SettingsBraveAccountRowElement.is, SettingsBraveAccountRowElement)
