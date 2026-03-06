// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '/strings.m.js'
import 'chrome://resources/brave/leo.bundle.js'

import { CrLitElement } from 'chrome://resources/lit/v3_0/lit.rollup.js'
import { I18nMixinLit } from 'chrome://resources/cr_elements/i18n_mixin_lit.js'
import { BraveOriginStartupHandler } from './brave_origin_startup.mojom-webui.js'

import { getCss } from './brave_origin_startup_app.css.js'
import { getHtml } from './brave_origin_startup_app.html.js'

const BraveOriginStartupAppElementBase = I18nMixinLit(CrLitElement)

export class BraveOriginStartupAppElement
    extends BraveOriginStartupAppElementBase {
  static get is() {
    return 'brave-origin-startup-app'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      currentView: { type: String },
      purchaseId: { type: String },
      verifying: { type: Boolean, reflect: true },
      error: { type: String },
    }
  }

  accessor currentView: 'main' | 'restore' = 'main'
  accessor purchaseId: string = ''
  accessor verifying: boolean = false
  accessor error: string = ''

  private handler_ = BraveOriginStartupHandler.getRemote()

  private onVisibilityChange_ = () => {
    if (document.visibilityState === 'visible') {
      this.checkPurchaseState_()
    }
  }

  override connectedCallback() {
    super.connectedCallback()
    document.addEventListener('visibilitychange', this.onVisibilityChange_)
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    document.removeEventListener('visibilitychange', this.onVisibilityChange_)
  }

  private async checkPurchaseState_() {
    const { isPurchased } = await this.handler_.checkPurchaseState()
    if (isPurchased) {
      this.handler_.closeDialog()
    }
  }

  onRestoreClick() {
    this.currentView = 'restore'
    this.error = ''
    this.purchaseId = ''
    this.focusPurchaseIdInput_()
  }

  onBuyClick() {
    this.handler_.openBuyWindow()
    this.currentView = 'restore'
    this.error = ''
    this.focusPurchaseIdInput_()
    this.purchaseId = ''
  }

  private focusPurchaseIdInput_() {
    this.updateComplete.then(() => {
      this.shadowRoot?.querySelector<HTMLInputElement>('input')?.focus()
    })
  }

  onPurchaseIdInput(e: Event) {
    this.purchaseId = (e.target as HTMLInputElement).value.trim()
    this.error = ''
  }

  async onVerifyClick() {
    if (!this.purchaseId || this.verifying) {
      return
    }
    this.verifying = true
    this.error = ''

    const { success, errorMessage } = await this.handler_.verifyPurchaseId(
      this.purchaseId,
    )
    this.verifying = false
    if (success) {
      this.handler_.closeDialog()
    } else {
      this.error = errorMessage
    }
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-origin-startup-app': BraveOriginStartupAppElement
  }
}

customElements.define(
  BraveOriginStartupAppElement.is,
  BraveOriginStartupAppElement,
)
