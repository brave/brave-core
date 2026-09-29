/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from '//resources/js/assert.js'
import {
  CrLitElement,
  PropertyValues,
} from '//resources/lit/v3_0/lit.rollup.js'

import { getCss } from './brave_account_row_common.css.js'
import { getHtml } from './brave_account_compact_row.html.js'
import { TextTruncator } from './brave_account_text_truncator.js'
import { routes } from './route.js'
import { Router } from './router.js'

// A row that only points at the account section, which owns the full account
// UI. The states it stands in for differ in their description alone.
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
      description: { type: String },
      truncatedDescription: { type: String, state: true },
    }
  }

  accessor description = ''
  protected accessor truncatedDescription = ''

  private truncator = new TextTruncator((text: string) => {
    this.truncatedDescription = text
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
      assert(element)
      this.truncator.observe(element, this.description)
    }
  }

  protected navigateToAccountSection() {
    Router.getInstance().navigateTo(routes.GET_STARTED)
  }

  protected onRowKeyDown(event: KeyboardEvent) {
    if (event.key !== 'Enter' && event.key !== ' ') return

    event.preventDefault()
    this.navigateToAccountSection()
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
