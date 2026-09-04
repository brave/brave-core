/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { AccountState } from './brave_account.mojom-webui.js'
import {
  BraveAccountRowBrowserProxy,
  BraveAccountRowBrowserProxyImpl,
} from './brave_account_row_browser_proxy.js'
import { getHtml } from './brave_account_settings.html.js'

// The page root for the account rows on Android/iOS, where Settings is native
// and so the rows are served as a page instead of being compiled into the
// brave://settings bundle. Mirrors SettingsBraveAccountRowElement, which is
// the desktop mount for the same rows.
export class BraveAccountSettingsElement extends CrLitElement {
  static get is() {
    return 'brave-account-settings'
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

  // Mobile has no ConstrainedWebDialog, so unlike the desktop mount, which
  // asks the browser to open one, authentication is reached by navigating to
  // the route that mounts the dialogs - see brave_account_route.ts.
  //
  // TODO: this reloads the document to swap one root element for another. Once
  // the flows live at their own route, mount them in place via history
  // navigation instead.
  protected onOpenBraveAccountDialog() {
    window.location.pathname = '/authentication'
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
    'brave-account-settings': BraveAccountSettingsElement
  }
}

customElements.define(
  BraveAccountSettingsElement.is,
  BraveAccountSettingsElement,
)
