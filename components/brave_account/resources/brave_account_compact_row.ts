/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import { loadTimeData } from '//resources/js/load_time_data.js'
import {
  CrLitElement,
  PropertyValues,
} from '//resources/lit/v3_0/lit.rollup.js'

import { AccountState } from './brave_account.mojom-webui.js'
import { BraveAccountCompactRowBrowserProxy } from './brave_account_compact_row_browser_proxy.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'
import { getCss } from './brave_account_compact_row.css.js'
import { getHtml } from './brave_account_compact_row.html.js'

// A row for pages that are not where the account is managed: it only links to
// the account section, which owns the full account UI. Every account state is
// summarized by its description alone.
export class BraveAccountCompactRowElement extends CrLitElement {
  static get is() {
    return 'brave-account-compact-row'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      actionRequired: { type: Boolean, state: true },
      description: { type: String, state: true },
      state: { type: Object, state: true },
    }
  }

  protected accessor actionRequired = false
  protected accessor description = ''
  protected accessor state: AccountState | undefined = undefined

  private browserProxy!: BraveAccountCompactRowBrowserProxy
  private accountStateListenerId: number | null = null

  override connectedCallback() {
    super.connectedCallback()

    this.browserProxy = new BraveAccountCompactRowBrowserProxy()
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
    this.browserProxy.authenticationObserverCallbackRouter.$.close()
  }

  override willUpdate(changedProperties: PropertyValues<this>) {
    super.willUpdate(changedProperties)

    if ((changedProperties as Map<PropertyKey, unknown>).has('state')) {
      this.actionRequired = Boolean(
        this.state?.loggedIn?.verification
          ?? this.state?.loggedOut?.verification,
      )
      this.description = this.getDescription()
    }
  }

  private getDescription() {
    if (this.state === undefined) {
      return ''
    }

    if (this.actionRequired) {
      return loadTimeData.getString(
        BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_ACTION_REQUIRED_LABEL,
      )
    }

    return (
      this.state.loggedIn?.email
      ?? loadTimeData.getString(
        BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_LOGGED_OUT_ROW_TITLE,
      )
    )
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-compact-row': BraveAccountCompactRowElement
  }
}

customElements.define(
  BraveAccountCompactRowElement.is,
  BraveAccountCompactRowElement,
)
