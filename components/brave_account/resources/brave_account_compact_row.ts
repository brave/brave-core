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
import {
  BraveAccountRowBrowserProxy,
  BraveAccountRowBrowserProxyImpl,
} from './brave_account_row_browser_proxy.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'
import { getCss } from './brave_account_compact_row.css.js'
import { getHtml } from './brave_account_compact_row.html.js'
import { TextTruncator } from './brave_account_text_truncator.js'
import { routes } from './route.js'
import { Router } from './router.js'

// A row for pages that are not where the account is managed: it only points at
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
      displayDescription: { type: String, state: true },
      state: { type: Object, state: true },
    }
  }

  protected accessor actionRequired = false
  protected accessor description = ''
  protected accessor displayDescription = ''
  protected accessor state: AccountState | undefined = undefined

  private browserProxy: BraveAccountRowBrowserProxy =
    new BraveAccountRowBrowserProxyImpl()
  private accountStateListenerId: number | null = null
  private truncator = new TextTruncator((text: string) => {
    this.displayDescription = text
  })

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

    this.truncator.disconnect()
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

  override updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties)

    if ((changedProperties as Map<PropertyKey, unknown>).has('description')) {
      const element =
        this.shadowRoot?.querySelector<HTMLElement>('.description')
      if (element) {
        this.truncator.observe(element, this.description)
      } else {
        this.truncator.disconnect()
      }
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

  protected navigateToAccountSection() {
    Router.getInstance().navigateTo(routes.GET_STARTED)
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
