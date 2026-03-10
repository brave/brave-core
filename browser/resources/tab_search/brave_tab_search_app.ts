// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './tab_focus/tab_focus_page.js'
import '//resources/cr_elements/cr_tabs/cr_tabs.js'

import { loadTimeData } from '//resources/js/load_time_data.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { TabSearchApiProxyImpl } from './tab_search_api_proxy.js'

import { getHtml } from './brave_tab_search_app.html.js'
import { getCss } from './brave_tab_search_app.css.js'

// Brave wrapper component that adds tab routing between the tab search page
// and the Leo AI tab organization page.
export class BraveTabSearchAppElement extends CrLitElement {
  static get is() {
    return 'brave-tab-search-app'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      availableHeight: { type: Number },
      tabOrganizationEnabled_: { type: Boolean },
      selectedTabIndex_: { type: Number },
    }
  }

  accessor availableHeight: number = 0
  protected accessor tabOrganizationEnabled_: boolean = loadTimeData.getBoolean(
    'tabOrganizationEnabled',
  )
  protected accessor selectedTabIndex_: number = 0

  private listenerIds_: number[] = []
  private visibilityChangedListener_: (() => void) | null = null

  protected get tabNames_(): string[] {
    return [
      loadTimeData.getString('tabSearchTabName'),
      loadTimeData.getString('tabOrganizationTabName'),
    ]
  }

  protected onTabSelectedChanged_(e: CustomEvent<{ value: number }>) {
    this.selectedTabIndex_ = e.detail.value
  }

  override connectedCallback() {
    super.connectedCallback()

    const apiProxy = TabSearchApiProxyImpl.getInstance()

    const callbackRouter = apiProxy.getCallbackRouter()
    this.listenerIds_.push(
      callbackRouter.tabOrganizationEnabledChanged.addListener(
        (enabled: boolean) => {
          this.tabOrganizationEnabled_ = enabled
        },
      ),
    )

    this.visibilityChangedListener_ = () => {
      if (document.visibilityState === 'visible') {
        apiProxy.maybeShowUi()
      }
    }
    document.addEventListener(
      'visibilitychange',
      this.visibilityChangedListener_,
    )
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    const callbackRouter =
      TabSearchApiProxyImpl.getInstance().getCallbackRouter()
    this.listenerIds_.forEach((id) => callbackRouter.removeListener(id))

    if (this.visibilityChangedListener_) {
      document.removeEventListener(
        'visibilitychange',
        this.visibilityChangedListener_,
      )
      this.visibilityChangedListener_ = null
    }
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-tab-search-app': BraveTabSearchAppElement
  }
}

customElements.define(BraveTabSearchAppElement.is, BraveTabSearchAppElement)
