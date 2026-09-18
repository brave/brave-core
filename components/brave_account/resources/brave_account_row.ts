/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { AccountState, DialogMode } from './brave_account.mojom-webui.js'
import {
  BraveAccountRowBrowserProxy,
  BraveAccountRowBrowserProxyImpl,
} from './brave_account_row_browser_proxy.js'
import { getHtml } from './brave_account_row.html.js'

// Mounts the account rows and feeds them account state. brave://settings
// compiles this in; Android and iOS, where Settings is native, get it from the
// page this WebUI serves at /settings.
export class BraveAccountRowElement extends CrLitElement {
  static get is() {
    return 'brave-account-row'
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

  // Where the flows are opened is the host's decision: brave://settings opens a
  // dialog over the page, mobile presents them over the page serving the rows.
  // Either way the browser does it, so both go through the same call.
  protected onOpenBraveAccountDialog(
    e: CustomEvent<{ initiatingServiceName: string; dialogMode: DialogMode }>,
  ) {
    this.browserProxy.dialogOpener.openDialog(
      e.detail.initiatingServiceName,
      e.detail.dialogMode,
    )
  }

  override connectedCallback() {
    super.connectedCallback()

    this.accountStateListenerId =
      this.browserProxy.authenticationObserverCallbackRouter.onAccountStateChanged.addListener(
        (state: AccountState) => {
          this.state = state
        },
      )
  }

  override disconnectedCallback() {
    super.disconnectedCallback()

    assert(this.accountStateListenerId)
    this.browserProxy.authenticationObserverCallbackRouter.removeListener(
      this.accountStateListenerId,
    )
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-row': BraveAccountRowElement
  }
}

customElements.define(
  BraveAccountRowElement.is,
  BraveAccountRowElement,
)
