/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  CrLitElement,
  PropertyValues,
} from '//resources/lit/v3_0/lit.rollup.js'

import { SettingsPage } from './brave_account.mojom-webui.js'
import { getCss } from './brave_account_link_row.css.js'
import { getHtml } from './brave_account_link_row.html.js'
import { TextTruncator } from './brave_account_text_truncator.js'
import { routes } from './route.js'
import { Router } from './router.js'

function getRoute(page: SettingsPage) {
  switch (page) {
    case SettingsPage.kBraveAccount:
      return routes.GET_STARTED
  }
}

export class BraveAccountLinkRowElement extends CrLitElement {
  static get is() {
    return 'brave-account-link-row'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      description: { type: String },
      displayDescription: { type: String, state: true },
      icon: { type: String },
      label: { type: String },
      page: { type: Number },
    }
  }

  accessor description = ''
  protected accessor displayDescription = ''
  accessor icon = ''
  accessor label = ''
  accessor page = SettingsPage.kBraveAccount

  private truncator = new TextTruncator((text: string) => {
    this.displayDescription = text
  })

  override disconnectedCallback() {
    super.disconnectedCallback()

    this.truncator.disconnect()
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

  protected openSettingsPage() {
    Router.getInstance().navigateTo(getRoute(this.page))
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-link-row': BraveAccountLinkRowElement
  }
}

customElements.define(BraveAccountLinkRowElement.is, BraveAccountLinkRowElement)
