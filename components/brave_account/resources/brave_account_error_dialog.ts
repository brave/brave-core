/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { getHtml } from './brave_account_error_dialog.html.js'
import { TaggedFailureReason } from './brave_account_dialogs.js'

export class BraveAccountErrorDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-error-dialog'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      failureReason: { type: Object },
    }
  }

  protected accessor failureReason!: TaggedFailureReason
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-error-dialog': BraveAccountErrorDialogElement
  }
}

customElements.define(
  BraveAccountErrorDialogElement.is,
  BraveAccountErrorDialogElement,
)
