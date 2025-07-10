/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getHtml } from './brave_account_dialogs.html.js'

export enum Dialog {
  NONE,
  CREATE,
  ENTRY,
  FORGOT_PASSWORD,
  SIGN_IN,
}

export class BraveAccountDialogs extends CrLitElement {
  static get is() {
    return 'brave-account-dialogs'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      dialog: { type: Dialog },
      signedIn: { type: Boolean, reflect: true },
    }
  }

  protected onBackButtonClicked() {
    switch (this.dialog) {
      case Dialog.CREATE:
        this.dialog = Dialog.ENTRY
        break
      case Dialog.FORGOT_PASSWORD:
        this.dialog = Dialog.SIGN_IN
        break
      case Dialog.SIGN_IN:
        this.dialog = Dialog.ENTRY
        break
    }
  }

  protected onCloseButtonClicked() {
    this.browserProxy.closeDialog()
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor dialog: Dialog = Dialog.ENTRY
  protected accessor signedIn: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-dialogs': BraveAccountDialogs
  }
}

customElements.define(BraveAccountDialogs.is, BraveAccountDialogs)
