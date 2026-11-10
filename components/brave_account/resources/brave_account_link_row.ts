/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  CrLitElement,
  PropertyValues,
} from '//resources/lit/v3_0/lit.rollup.js'

import {
  // <if expr="is_android or is_ios">
  SettingsOpener,
  SettingsOpenerRemote,
  // </if>
  SettingsPage,
} from './brave_account.mojom-webui.js'
import { getCss } from './brave_account_link_row.css.js'
import { getHtml } from './brave_account_link_row.html.js'
import { TextTruncator } from './brave_account_text_truncator.js'
// <if expr="not is_android and not is_ios">
import { routes } from './route.js'
import { Router } from './router.js'
// </if>

// <if expr="not is_android and not is_ios">
function getRoute(page: SettingsPage) {
  switch (page) {
    case SettingsPage.kBraveAccount:
      return routes.GET_STARTED
  }
}
// </if>

export const CARD_CLASS = [
  // <if expr="not is_android and not is_ios">
  '',
  // </if>
  // <if expr="is_android or is_ios">
  'card',
  // </if>
][0]

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

  // <if expr="is_android or is_ios">
  private settingsOpener: SettingsOpenerRemote = SettingsOpener.getRemote()
  // </if>

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
    // <if expr="not is_android and not is_ios">
    Router.getInstance().navigateTo(getRoute(this.page))
    // </if>
    // <if expr="is_android or is_ios">
    this.settingsOpener.openSettings(this.page)
    // </if>
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-link-row': BraveAccountLinkRowElement
  }
}

customElements.define(BraveAccountLinkRowElement.is, BraveAccountLinkRowElement)
